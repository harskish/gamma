#include "SPH.hpp"
#include "utils.hpp"
#include "GLProgram.hpp"
#include "Camera.hpp"
#include <imgui.h>

static const char* integrator = "Leapfrog";

namespace SPH {
    SPHSimulator::SPHSimulator(void) {        
        //clt::setCpuDebug(true);
        setup();
    }

    void SPHSimulator::update() {
        glFinish();
        glCheckError();

        try {
            clState.cmdQueue.enqueueAcquireGLObjects(&kernelData.sharedMemory);

            // Hash grid
            clState.cmdQueue.enqueueNDRangeKernel(cellIdxKernel, cl::NullRange, cl::NDRange(kernelData.numParticles)); // get flat cell indices
            clState.cmdQueue.enqueueNDRangeKernel(clearOffsetsKernel, cl::NullRange, cl::NDRange(kernelData.numCells)); // clear offset list
            
            // Sort
            vex::sort_by_key(kernelData.vexCellIndices, kernelData.vexParticleIndices, vex::less<cl_uint>()); // sort particles by cell index
            clState.cmdQueue.enqueueNDRangeKernel(calcOffsetsKernel, cl::NullRange, cl::NDRange(kernelData.numParticles)); // create new offset list

            // Core SPH kernels
            clState.cmdQueue.enqueueNDRangeKernel(densityKernel, cl::NullRange, cl::NDRange(kernelData.numParticles));
            clState.cmdQueue.enqueueNDRangeKernel(forceKernel, cl::NullRange, cl::NDRange(kernelData.numParticles));
            
            // Integrate
            clt::Kernel& integrateKernel = !strcmp(integrator, "S. Euler") ?
                (clt::Kernel&)eulerKernel : (iteration == 0) ? leapfrogStartKernel : leapfrogKernel;
            clState.cmdQueue.enqueueNDRangeKernel(integrateKernel, cl::NullRange, cl::NDRange(kernelData.numParticles));
            
            // Give buffers back to OpenGL for drawing
            clState.cmdQueue.enqueueReleaseGLObjects(&kernelData.sharedMemory);
            clState.cmdQueue.finish();

            iteration++;
        }
        catch (cl::Error &e) {
            std::cout << "Error in " << e.what() << std::endl;
            clt::check(e.err(), "Failed to execute SPH kernels");
        }
        
        // DEBUG UI
        drawUI();
    }

    void SPHSimulator::drawUI() {
        auto rebuildKernels = [&]() {
            for (clt::Kernel* kernel : getKernels()) {
                kernel->rebuild(true);
            }
        };

        static bool show = true;
        if (!ImGui::Begin("SPH settings", &show)) {
            ImGui::End();
            return;
        }

        if (ImGui::SliderFloat("p0", &kernelData.p0, 0.1f, 20.0f, "%.3f", 3.0f)) {
            densityKernel.setArg("restDensity", kernelData.p0);
            forceKernel.setArg("restDensity", kernelData.p0);
        }

        if (ImGui::SliderFloat("sm.rad", &kernelData.smoothingRadius, 0.1f, 2.0f, "%.4f", 3.0f)) {
            densityKernel.setArg("smoothingRadius", kernelData.smoothingRadius);
            forceKernel.setArg("smoothingRadius", kernelData.smoothingRadius);
            cellIdxKernel.setArg("smoothingRadius", kernelData.smoothingRadius);
        }

        if (ImGui::SliderFloat("k_press", &kernelData.K, 0.0f, 400.0f, "%.4f", 3.0f)) {
            forceKernel.setArg("kPressure", kernelData.K);
        }

        if (ImGui::SliderFloat("k_visc", &kernelData.eps, 0.0f, 1.5f, "%.4f", 3.0f)) {
            forceKernel.setArg("kViscosity", kernelData.eps);
        }

        if (ImGui::SliderFloat("mass", &kernelData.particleMass, 0.01f, 5.0f, "%.4f", 3.0f)) {
            densityKernel.setArg("particleMass", kernelData.particleMass);
            forceKernel.setArg("particleMass", kernelData.particleMass);
        }

        if (ImGui::SliderFloat("box", &kernelData.boxSize, 2.0f, 20.0f, "%.4f", 1.0f)) {
            eulerKernel.setArg("boxSize", kernelData.boxSize);
            leapfrogKernel.setArg("boxSize", kernelData.boxSize);
            leapfrogStartKernel.setArg("boxSize", kernelData.boxSize);
        }

        static bool eos = (bool)kernelData.EOS;
        if (ImGui::RadioButton("Advanced EOS", &eos)) {
            eos = !eos;
            kernelData.EOS = (cl_int)eos;
            rebuildKernels();
            std::cout << "EOS: " << kernelData.EOS << std::endl;
        }

        // Neighborhoor iteration method
        const char* nbMethods[] = { "FOREACH_NEIGHBOR_GRID", "FOREACH_NEIGHBOR_GRID_SAFE", "FOREACH_NEIGHBOR_NAIVE" };
        static const char* currentNBMethod = nbMethods[0];
        if (ImGui::BeginCombo("##combo", currentNBMethod)) // The second parameter is the label previewed before opening the combo.
        {
            for (int n = 0; n < IM_ARRAYSIZE(nbMethods); n++)
            {
                bool is_selected = (currentNBMethod == nbMethods[n]); // You can store your selection however you want, outside or inside your objects
                if (ImGui::Selectable(nbMethods[n], is_selected)) {
                    currentNBMethod = nbMethods[n];
                    kernelData.foreachFunString = std::string(currentNBMethod);
                    rebuildKernels();
                }
            }
            ImGui::EndCombo();  
        }

        // Integrator
        const char* integrators[] = { "Leapfrog", "S. Euler" };
        if (ImGui::BeginCombo("##integrator", integrator))
        {
            for (int n = 0; n < IM_ARRAYSIZE(integrators); n++)
            {
                bool is_selected = (integrator == integrators[n]);
                if (ImGui::Selectable(integrators[n], is_selected)) {
                    integrator = integrators[n];
                    iteration = 0; // force leapfrog start
                }
                    
            }
            ImGui::EndCombo();
        }

        ImGui::End();
    }

    // Selection sort for debug purposes
    void SPHSimulator::debugSortCPU() {
        auto N = kernelData.numParticles;
        if (N > 1000)
            throw std::runtime_error("Using debug sort on too many particles!");

        auto keys = kernelData.vexCellIndices.map(0);
        auto vals = kernelData.vexParticleIndices.map(0);
        for (size_t s = 0; s < N; s++) {
            for (size_t e = s + 1; e < N; e++) {
                cl_uint ke = keys[e];
                cl_uint ks = keys[s];
                if (ke < ks) {
                    cl_uint tmp;

                    tmp = keys[e];
                    keys[e] = keys[s];
                    keys[s] = tmp;

                    tmp = vals[e];
                    vals[e] = vals[s];
                    vals[s] = tmp;
                }
            }
        }
    }

    void SPHSimulator::render(const CameraBase* camera) {
        GLProgram* prog = getProgram("Render::SPH_spheres", "sph_spheres.vert", "sph_spheres.frag");

        const float R = kernelData.particleSize * 200.0f;

        const glm::mat4 M(1.0f);
        const glm::mat4 P = camera->getP();
        const glm::mat4 V = camera->getV();
        const glm::vec3 sunPos(kernelData.sunPosition.x,
            kernelData.sunPosition.y, kernelData.sunPosition.z);

        prog->use();
        prog->setUniform("M", M);
        prog->setUniform("V", V);
        prog->setUniform("P", P);
        prog->setUniform("sunPosition", sunPos);
        prog->setUniform("pointRadius", R);

        particles->bind();
        glDrawArrays(GL_POINTS, 0, (GLsizei)kernelData.numParticles);
        particles->unbind();

        glCheckError();
    }

    void SPHSimulator::renderUnshaded() {
        // Particles don't cast shadows for now
        return;
    }
    
    void SPHSimulator::setup() {
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

        static_assert((sizeof(cl_float) == sizeof(GLfloat)) &&
            (alignof(cl_float) == alignof(GLfloat)), "Incompatible float types");
        static_assert((sizeof(cl_float4) == sizeof(glm::vec4)) &&
            (alignof(cl_float4) == alignof(glm::vec4)), "Incompatible float4 types");

        std::vector<glm::vec4> vel;
        std::vector<glm::vec4> pos;

        
        initGrid(pos, vel); // regular grid using given particle count
        //initRandom(pos, vel); // random positions
        //initIndicator(pos, vel); // indicator function init, final particle count unknown

        kernelData.numParticles = pos.size();
        std::cout << "Particles: " << kernelData.numParticles << std::endl;

        // Init GL objects
        positions.reset(new VertexBuffer());
        velocities.reset(new VertexBuffer());
        particles.reset(new VertexArray());
        glCheckError();

        particles->bind();
        glBindBuffer(GL_ARRAY_BUFFER, positions->id);
        glBufferData(GL_ARRAY_BUFFER, pos.size() * 4 * sizeof(GLfloat), pos.data(), GL_DYNAMIC_DRAW);
        glCheckError();

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, (void*)0); // zero stride + zero offset = tightly packed
        glCheckError();

        glBindBuffer(GL_ARRAY_BUFFER, velocities->id);
        glBufferData(GL_ARRAY_BUFFER, vel.size() * 4 * sizeof(GLfloat), vel.data(), GL_DYNAMIC_DRAW);
        glCheckError();

        particles->unbind();

        // VexCL buffers must be divisible by 11 to sort correctly (???)
        cl_uint vexBufferLen = 11 * (((kernelData.numParticles - 1) / 11) + 1);

        cl_int err = 0;
        try {
            kernelData.clPositions = cl::BufferGL(clState.context, CL_MEM_READ_WRITE, positions->id, &err);
            kernelData.clVelocities = cl::BufferGL(clState.context, CL_MEM_READ_WRITE, velocities->id, &err);
            kernelData.clVelocityHalf = cl::Buffer(clState.context, CL_MEM_READ_WRITE,
                kernelData.numParticles * sizeof(cl_float4), nullptr, &err); // half-step-aligned velocities for leapfrog

            kernelData.sharedMemory.clear();
            kernelData.sharedMemory.push_back(kernelData.clPositions);
            kernelData.sharedMemory.push_back(kernelData.clVelocities);

            kernelData.clDensities = cl::Buffer(clState.context, CL_MEM_READ_WRITE, kernelData.numParticles * sizeof(cl_float), nullptr, &err);
            kernelData.clForces = cl::Buffer(clState.context, CL_MEM_READ_WRITE, kernelData.numParticles * sizeof(cl_float4), nullptr, &err);

            kernelData.particleIndices = cl::Buffer(clState.context, CL_MEM_READ_WRITE, vexBufferLen * sizeof(cl_uint), nullptr, &err);
            kernelData.cellIndices = cl::Buffer(clState.context, CL_MEM_READ_WRITE, vexBufferLen * sizeof(cl_uint), nullptr, &err); // mapped to range [0, numCells]
            kernelData.offsetList = cl::Buffer(clState.context, CL_MEM_READ_WRITE, kernelData.numCells * sizeof(cl_uint), nullptr, &err);

            // VexCL buffers for hash grid
            kernelData.vexParticleIndices = vex::vector<cl_uint>({ clState.cmdQueue }, kernelData.particleIndices);
            kernelData.vexCellIndices = vex::vector<cl_uint>({ clState.cmdQueue }, kernelData.cellIndices);
        }
        catch (cl::Error &e) {
            clt::check(e.err(), "Failed to allocate SPH buffers");
        }
        
        // Initialize particle indices
        std::vector<cl_uint> tmp(vexBufferLen);
        std::iota(tmp.begin(), tmp.end(), 0);
        vex::copy(tmp, kernelData.vexParticleIndices);

        // Initialize cell indices
        std::fill(tmp.begin(), tmp.end(), std::numeric_limits<cl_uint>::max());
        vex::copy(tmp, kernelData.vexCellIndices);

        // All kernel args have now been initalized
        buildKernels();

        // Normalize particle state (mass/rest density)
        normalizeMass();
    }

    std::vector<clt::Kernel*> SPHSimulator::getKernels() {
        return {
            &cellIdxKernel,
            &clearOffsetsKernel,
            &calcOffsetsKernel,
            &eulerKernel,
            &leapfrogKernel,
            &leapfrogStartKernel,
            &densityKernel,
            &forceKernel
        };
    }

    void SPHSimulator::initRandom(std::vector<glm::vec4>& pos, std::vector<glm::vec4>& vel) {
        for (cl_uint i = 0; i < kernelData.numParticles; i++) {
            float x = (float)rand() / RAND_MAX;
            float y = (float)rand() / RAND_MAX;
            float z = (float)rand() / RAND_MAX;
            pos.push_back(glm::vec4(x, y, z, 0.0f));
            vel.push_back(glm::vec4(0.0f));
        }
    }

    void SPHSimulator::initGrid(std::vector<glm::vec4>& pos, std::vector<glm::vec4>& vel) {
        // Integer Kth root of n using Newton's method
        auto iroot = [](cl_uint n, cl_uint k) {
            cl_uint u = n, s = n + 1;
            while (u < s) {
                s = u;
                cl_uint t = (k - 1) * s + n / (cl_uint)glm::pow(s, k - 1);
                u = t / k;
            }
            return s;
        };

        // Closest integer Kth root of n (can round up)
        auto ciroot = [](cl_uint n, cl_uint k) {
            return (cl_uint)glm::round(glm::pow(n, 1.0 / k));
        };

        const cl_uint k = kernelData.dims;
        const cl_uint Nside = ciroot(kernelData.numParticles, k);
        const cl_uint Nx = ((k > 0) ? Nside : 1);
        const cl_uint Ny = ((k > 1) ? Nside : 1);
        const cl_uint Nz = ((k > 2) ? Nside : 1);

        const float hh = kernelData.smoothingRadius / 1.3f;
        const float d = Nside * hh;
        for (cl_uint x = 0; x < Nx; x++) {
            for (cl_uint y = 0; y < Ny; y++) {
                for (cl_uint z = 0; z < Nz; z++) {
                    glm::vec3 p = glm::vec3(-d / 2) + d * glm::vec3(x, y, z) / glm::vec3(Nx, Ny, Nz);
                    pos.push_back(glm::vec4(p, 0.0f));
                    vel.push_back(glm::vec4(0.0f));
                }
            }
        }
    }

    // Unit box assumed for now
    void SPHSimulator::initIndicator(std::vector<glm::vec4>& pos, std::vector<glm::vec4>& vel) {
        auto boxIndicator = [](glm::vec3 p) {
            return (p.x < 0.5f) && (p.y < 0.5f) && (p.z < 0.5f);
        };

        auto sphereIndicator = [](glm::vec3 p) {
            glm::vec3 orig(0.0f);
            glm::vec3 d = p - orig;
            return glm::length(d) < 0.7f;
        };

        auto indicator = sphereIndicator;
        float hh = kernelData.smoothingRadius / 1.3f;

        for (float x = 0; x < 1; x += hh) {
            for (float y = 0; y < 1; y += hh) {
                for (float z = 0; z < 1; z += hh) {
                    glm::vec3 p(x, y, z);
                    if (indicator(p)) {
                        pos.push_back(glm::vec4(p, 0.0f));
                        vel.push_back(glm::vec4(0));
                    }
                }
            }
        }
    }

    void SPHSimulator::normalizeMass() {
        if (!densityKernel)
            throw std::runtime_error("Must build kernels before running mass normalization");

        glFinish();
        glCheckError();

        // Acquire densities from device
        std::unique_ptr<cl_float[]> densities(new cl_float[kernelData.numParticles]);

        // Compute densities of fluid particles
        // No integration is performed
        try {
            // Update grid
            clState.cmdQueue.enqueueAcquireGLObjects(&kernelData.sharedMemory);
            clState.cmdQueue.enqueueNDRangeKernel(cellIdxKernel, cl::NullRange, cl::NDRange(kernelData.numParticles));
            clState.cmdQueue.enqueueNDRangeKernel(clearOffsetsKernel, cl::NullRange, cl::NDRange(kernelData.numCells));
            vex::sort_by_key(kernelData.vexCellIndices, kernelData.vexParticleIndices, vex::less<cl_uint>());
            clState.cmdQueue.enqueueNDRangeKernel(calcOffsetsKernel, cl::NullRange, cl::NDRange(kernelData.numParticles));
            
            // Compute density
            clState.cmdQueue.enqueueNDRangeKernel(densityKernel, cl::NullRange, cl::NDRange(kernelData.numParticles));
            clState.cmdQueue.enqueueReadBuffer(kernelData.clDensities, CL_FALSE, 0, kernelData.numParticles * sizeof(cl_float), densities.get());
            clState.cmdQueue.enqueueReleaseGLObjects(&kernelData.sharedMemory);
            clState.cmdQueue.finish();
        }
        catch (cl::Error &e) {
            std::cout << "Error in " << e.what() << std::endl;
            clt::check(e.err(), "Failed to run fluid mass normalization");
        }

        cl_float rho0 = kernelData.p0;
        cl_float rho2s = 0.0f;
        cl_float rhos = 0.0f;
        for (int i = 0; i < kernelData.numParticles; i++) {
            cl_float rho = densities[i];
            rho2s += rho * rho;
            rhos += rho;
        }

        // Adjust particle mass
        kernelData.particleMass *= (rho0*rhos / rho2s);
        std::cout << "Particle mass set to " << kernelData.particleMass << std::endl;

        // Update mass in kernel arguments and preprocessor defs
        for (clt::Kernel* kernel : getKernels()) {
            kernel->rebuild(true);
        }
    }

    void SPHSimulator::buildKernels() {
        try {
            bool isIntel = contains(clState.platform.getInfo<CL_PLATFORM_VENDOR>(), "Intel");
            bool isCPU = clState.device.getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_CPU;

            for (clt::Kernel* kernel : getKernels()) {
                kernel->build(clState.context, clState.device, clState.platform);

                // Verify that kernel vectorization worked
                if (isIntel && isCPU) {
                    const std::string log = kernel->getBuildLog();
                    bool isDebug = clt::isCpuDebug();
                    bool binaryLoaded = contains(log, "Reload Program Binary Object");
                    bool vectorized = contains(log, "successfully vectorized");
                    
                    if (!(isDebug || binaryLoaded || vectorized))
                        throw std::runtime_error("Kernel vectorization failed!");
                }
            }
        }
        catch (std::runtime_error& e) {
            std::cout << "[SPH] Kernel building failed: " << e.what() << std::endl;
            exit(-1);
        }
    }

}
