#include "SPH.hpp"
#include "utils.hpp"
#include <exception>

namespace SPH {
    SPHSimulator::SPHSimulator(void) {
        setup();
    }

    void SPHSimulator::update() {
        clState.cmdQueue.enqueueNDRangeKernel(neighborKernel, cl::NullRange, cl::NDRange(simParams.nuMParticles));
        clState.cmdQueue.finish();
    }

    void SPHSimulator::render() {
        throw std::runtime_error("Unimplemented");
    }
    
    void SPHSimulator::setup() {
        setupCL();
        buildKernels();

        static_assert(sizeof(cl_float) == sizeof(GLfloat), "Incompatible float types");
        static_assert(sizeof(cl_float4) == 4 * sizeof(GLfloat), "Incompatible float types");
        
        // Regular grid of particles
        const float d = 1.0f;
        const float dp = d / simParams.nuMParticles;

        std::vector<cl_float4> vel;
        std::vector<cl_float4> pos;

        if (simParams.dims == 2) {
            const cl_uint Nside = (cl_uint)std::floor(std::pow(simParams.nuMParticles, 1.0 / 2.0));
            for (cl_uint x = 0; x < Nside; x++) {
                for (cl_uint y = 0; y < Nside; y++) {
                    cl_float4 p = { -d / 2 + x * dp, -d / 2 + y * dp, 0.0, 0.0 };
                    pos.push_back(p);
                    vel.push_back({ 0.0f, 0.0f, 0.0f, 0.0f });
                }
            }    
        }
        else if (simParams.dims == 3) {
            const cl_uint Nside = (cl_uint)std::floor(std::pow(simParams.nuMParticles, 1.0 / 3.0));
            for (cl_uint x = 0; x < Nside; x++) {
                for (cl_uint y = 0; y < Nside; y++) {
                    for (cl_uint z = 0; z < Nside; z++) {
                        cl_float4 p = { -d / 2 + x * dp, -d / 2 + y * dp, -d / 2 + z * dp, 0.0 };
                        pos.push_back(p);
                        vel.push_back({ 0.0f, 0.0f, 0.0f, 0.0f });
                    }
                }
            }
        }
        else {
            throw std::runtime_error("Incorrect SPH simulation dimensionality");
        }

        // Init GL objects
        particles.bind();
        glBindBuffer(GL_ARRAY_BUFFER, positions.id);
        glBufferData(GL_ARRAY_BUFFER, pos.size() * 4 * sizeof(GLfloat), pos.data(), GL_DYNAMIC_DRAW);
        glCheckError();

        glBindBuffer(GL_ARRAY_BUFFER, velocities.id);
        glBufferData(GL_ARRAY_BUFFER, vel.size() * 4 * sizeof(GLfloat), vel.data(), GL_DYNAMIC_DRAW);
        glCheckError();

        /*
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
        glCheckError();

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
        glCheckError();

        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
        glCheckError();
        */

        particles.unbind();

        cl_int err = 0;
        clPositions = cl::BufferGL(clState.context, CL_MEM_READ_WRITE, positions.id, &err);
        clt::check(err, "Could not create cl positions buffer");
        clVelocities = cl::BufferGL(clState.context, CL_MEM_READ_WRITE, velocities.id, &err);
        clt::check(err, "Could not create cl velocities buffer");

        sharedMemory.clear();
        sharedMemory.push_back(clPositions);
        sharedMemory.push_back(clVelocities);
    }

    void SPHSimulator::setupCL() {
        clState = clt::initialize("Intel", "i7");
        if (!clState.hasGLInterop)
            throw std::runtime_error("[SPH] Could not initialize CL-GL sharing");

        clt::setCpuDebug(false);
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
