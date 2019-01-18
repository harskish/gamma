#include "SPH.hpp"
#include "utils.hpp"
#include "GLProgram.hpp"
#include "Camera.hpp"

namespace SPH {
    SPHSimulator::SPHSimulator(void) {
        setup();
    }

    void SPHSimulator::update() {
        glFinish();
        glCheckError();

        cl_int err = 0;
        err |= clState.cmdQueue.enqueueAcquireGLObjects(&kernelData.sharedMemory);
        err |= clState.cmdQueue.enqueueNDRangeKernel(integrateKernel, cl::NullRange, cl::NDRange(kernelData.numParticles));
        err |= clState.cmdQueue.enqueueReleaseGLObjects(&kernelData.sharedMemory);
        err |= clState.cmdQueue.finish();
        clt::check(err, "Failed to execute SPH neighbor kernel");
    }

    void SPHSimulator::render(const CameraBase* camera) {
        GLProgram* prog = getProgram("Render::SPH_spheres", "sph_spheres.vert", "sph_spheres.frag");

        const float R = kernelData.particleSize;

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
        setupCL();
        glEnable(GL_VERTEX_PROGRAM_POINT_SIZE);

        static_assert(sizeof(cl_float) == sizeof(GLfloat), "Incompatible float types");
        static_assert(sizeof(cl_float4) == sizeof(glm::vec4), "Incompatible float4 types");
        
        // Regular grid of particles
        const float d = 0.5f;

        std::vector<glm::vec4> vel;
        std::vector<glm::vec4> pos;

        auto explode = [&](glm::vec4 pos) {
            glm::vec3 dir = glm::cross(kernelData.sunPosition - glm::vec3(pos), glm::vec3(0.0f, 1.0f, 0.0f));
            float vel = sqrt(kernelData.sunMass * 6.674e-11f / std::pow(glm::length(dir), 1.0f/2.0f));
            return glm::vec4(normalize(dir) * vel, 0.0f);
        };

        if (kernelData.dims == 2) {
            const cl_uint Nside = (cl_uint)std::pow(kernelData.numParticles, 1.0 / 2.0);
            const float dp = d / Nside;
            for (cl_uint x = 0; x < Nside; x++) {
                for (cl_uint y = 0; y < Nside; y++) {
                    glm::vec4 p(-d / 2 + x * dp, -d / 2 + y * dp, 0.0, 0.0);
                    pos.push_back(p);
                    vel.push_back(explode(p));
                }
            }    
        }
        else if (kernelData.dims == 3) {
            const cl_uint Nside = (cl_uint)std::pow(kernelData.numParticles, 1.0 / 3.0);
            const float dp = d / Nside;
            for (cl_uint x = 0; x < Nside; x++) {
                for (cl_uint y = 0; y < Nside; y++) {
                    for (cl_uint z = 0; z < Nside; z++) {
                        glm::vec4 p(-d / 2 + x * dp, -d / 2 + y * dp, -d / 2 + z * dp, 0.0);
                        pos.push_back(p);
                        vel.push_back(explode(p));
                    }
                }
            }
        }
        else {
            throw std::runtime_error("Incorrect SPH simulation dimensionality");
        }

        kernelData.numParticles = pos.size();

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

        cl_int err = 0;
        kernelData.clPositions = cl::BufferGL(clState.context, CL_MEM_READ_WRITE, positions->id, &err);
        clt::check(err, "Could not create cl positions buffer");
        kernelData.clVelocities = cl::BufferGL(clState.context, CL_MEM_READ_WRITE, velocities->id, &err);
        clt::check(err, "Could not create cl velocities buffer");

        kernelData.sharedMemory.clear();
        kernelData.sharedMemory.push_back(kernelData.clPositions);
        kernelData.sharedMemory.push_back(kernelData.clVelocities);

        // All kernel args have now been initalized
        buildKernels();
    }

    void SPHSimulator::setupCL() {
        clState = clt::initialize("NVIDIA", "750");
        if (!clState.hasGLInterop)
            throw std::runtime_error("[SPH] Could not initialize CL-GL sharing");

        clt::setCpuDebug(false);
        clt::setKernelCacheDir("Gamma/Cache/kernel_binaries");
    }

    void SPHSimulator::buildKernels() {
        clt::Kernel* kernels[] = { &neighborKernel, &integrateKernel };

        try {
            for (clt::Kernel* kernel : kernels) {
                kernel->build(clState.context, clState.device, clState.platform);
            }
        }
        catch (std::runtime_error& e) {
            std::cout << "[SPH] Kernel building failed: " << e.what() << std::endl;
            exit(-1);
        }
    }

}
