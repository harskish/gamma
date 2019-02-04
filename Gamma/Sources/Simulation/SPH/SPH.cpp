#include "SPH.hpp"
#include "utils.hpp"
#include "GLProgram.hpp"
#include "Camera.hpp"
#include <imgui.h>

namespace SPH {
    SPHSimulator::SPHSimulator(void) {
        //clt::setCpuDebug(true);

        setup();
    }

    void SPHSimulator::update() {
        glFinish();
        glCheckError();

        try {
            cl_int err = 0;
            err |= clState.cmdQueue.enqueueAcquireGLObjects(&kernelData.sharedMemory);

            // Hash grid
            err |= clState.cmdQueue.enqueueNDRangeKernel(cellIdxKernel, cl::NullRange, cl::NDRange(kernelData.numParticles)); // get flat cell indices
            err |= clState.cmdQueue.enqueueNDRangeKernel(clearOffsetsKernel, cl::NullRange, cl::NDRange(kernelData.numCells)); // clear offset list
            vex::sort_by_key(kernelData.vexCellIndices, kernelData.vexParticleIndices, vex::less_equal<cl_uint>()); // sort particles by cell index
            err |= clState.cmdQueue.enqueueNDRangeKernel(calcOffsetsKernel, cl::NullRange, cl::NDRange(kernelData.numParticles)); // create new offset list

            // Rest of simulation
            err |= clState.cmdQueue.enqueueNDRangeKernel(densityKernel, cl::NullRange, cl::NDRange(kernelData.numParticles));
            err |= clState.cmdQueue.enqueueNDRangeKernel(forceKernel, cl::NullRange, cl::NDRange(kernelData.numParticles));
            err |= clState.cmdQueue.enqueueNDRangeKernel(integrateKernel, cl::NullRange, cl::NDRange(kernelData.numParticles));
            
            // Give buffers back to OpenGL for drawing
            err |= clState.cmdQueue.enqueueReleaseGLObjects(&kernelData.sharedMemory);
            err |= clState.cmdQueue.finish();
        }
        catch (cl::Error &e) {
            std::cout << "Error in " << e.what() << std::endl;
            clt::check(e.err(), "Failed to execute SPH kernels");
        }

        // TEST: print offset buffer
        {
            int activeCells = 0;
            vex::vector<cl_uint> X({ clState.cmdQueue }, kernelData.offsetList);
            auto mapped_ptr = X.map(0);
            for (size_t i = 0; i < X.part_size(0); ++i) {
                cl_uint val = mapped_ptr[i];
                if (val != (cl_uint)0xFFFFFFFF) {
                    printf("Offset %u: %u\n", i, val);
                    activeCells++;
                }
            }
            std::cout << "Active cells: " << activeCells << std::endl << std::endl;
        }
        

        // DEBUG UI
        drawUI();
    }

    void SPHSimulator::drawUI() {
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

        if (ImGui::SliderFloat("box", &kernelData.boxSize, 2.0f, 20.0f, "%.4f", 3.0f)) {
            integrateKernel.setArg("boxSize", kernelData.boxSize);
        }

        static bool eos = (bool)kernelData.EOS;
        if (ImGui::RadioButton("Advanced EOS", &eos)) {
            eos = !eos;
            kernelData.EOS = (cl_int)eos;
            forceKernel.rebuild(true);
            std::cout << "EOS: " << kernelData.EOS << std::endl;
        }

        ImGui::End();
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

        static_assert(sizeof(cl_float) == sizeof(GLfloat), "Incompatible float types");
        static_assert(sizeof(cl_float4) == sizeof(glm::vec4), "Incompatible float4 types");
        
        // Regular grid of particles
        const float d = 5.0;

        std::vector<glm::vec4> vel;
        std::vector<glm::vec4> pos;

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

        //// Random positions
        //for (cl_uint i = 0; i < kernelData.numParticles; i++) {
        //    float x = (float)rand() / RAND_MAX;
        //    float y = (float)rand() / RAND_MAX;
        //    float z = (float)rand() / RAND_MAX;
        //    pos.push_back(glm::vec4(x, y, z, 0.0f));
        //    vel.push_back(glm::vec4(0.0f));
        //}
        
        // Create K-dimensional grid
        const cl_uint k = kernelData.dims;
        const cl_uint Nside = ciroot(kernelData.numParticles, k); //iroot(kernelData.numParticles, k);
        const cl_uint Nx = ((k > 0) ? Nside : 1);
        const cl_uint Ny = ((k > 1) ? Nside : 1);
        const cl_uint Nz = ((k > 2) ? Nside : 1);
        
        for (cl_uint x = 0; x < Nx; x++) {
            for (cl_uint y = 0; y < Ny; y++) {
                for (cl_uint z = 0; z < Nz; z++) {
                    glm::vec3 p = glm::vec3(-d / 2) + d * glm::vec3(x, y, z) / glm::vec3(Nx, Ny, Nz);
                    pos.push_back(glm::vec4(p, 0.0f));
                    vel.push_back(glm::vec4(0.0f));
                }
            }
        }

        kernelData.numParticles = pos.size();
        std::cout << "Particles: " << kernelData.numParticles << std::endl;

        // Calculate rest density
        const float V = glm::pow(d, kernelData.dims);
        const float M = kernelData.numParticles * kernelData.particleMass;
        kernelData.p0 = M / V;

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

            kernelData.sharedMemory.clear();
            kernelData.sharedMemory.push_back(kernelData.clPositions);
            kernelData.sharedMemory.push_back(kernelData.clVelocities);

            kernelData.clForces = cl::Buffer(clState.context, CL_MEM_READ_WRITE, kernelData.numParticles * sizeof(cl_float4), nullptr, &err);
            kernelData.clDensities = cl::Buffer(clState.context, CL_MEM_READ_WRITE, kernelData.numParticles * sizeof(cl_float), nullptr, &err);

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
    }

    void SPHSimulator::buildKernels() {
        clt::Kernel* kernels[] = { &cellIdxKernel, &clearOffsetsKernel, &calcOffsetsKernel, &integrateKernel, &densityKernel, &forceKernel };

        try {
            bool isIntel = contains(clState.platform.getInfo<CL_PLATFORM_VENDOR>(), "Intel");

            for (clt::Kernel* kernel : kernels) {
                kernel->build(clState.context, clState.device, clState.platform);

                // Verify that kernel vectorization worked
                if (isIntel) {
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
