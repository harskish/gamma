#pragma once

#include <clt.hpp>
#include "ParticleSystem.hpp"
#include "SPHKernels.hpp"
#include "GLWrappers.hpp"

namespace SPH {

class SPHSimulator : public ParticleSystem {
public:
    SPHSimulator(void);

    void update() override;
    void render() override;

private:
    void setup();
    void setupCL();
    void buildKernels();

    struct {
        cl_uint nuMParticles = 1U;
        cl_uint dims = 3; // simulation dimensionality
    } simParams;

    clt::State clState;

    cl::BufferGL clPositions;
    cl::BufferGL clVelocities; // shared for visualzation
    std::vector<cl::Memory> sharedMemory;

    FindNeighborsKernel neighborKernel;
    TimeIntegrateKernel integrateKernel;

};

}
