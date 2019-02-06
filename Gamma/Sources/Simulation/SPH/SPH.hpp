#pragma once

#include "ParticleSystem.hpp"
#include "SPHKernels.hpp"
#include <clt.hpp>
#include <vexcl/vexcl.hpp>
#include <memory>

class CameraBase;

namespace SPH {

class SPHSimulator : public GPUParticleSystem {
public:
    SPHSimulator(void);
    ~SPHSimulator(void) {
        std::cout << "[SPH] Shutting down" << std::endl;
    }

    void update() override;
    void render(const CameraBase* camera) override;
    void renderUnshaded() override;
    void drawUI();
    void debugSortCPU();

private:
    void setup();
    void buildKernels();

    // Hash grid
    CalcCellIndexKernel cellIdxKernel; // flat cell indices, before sort
    ClearOffsetKernel clearOffsetsKernel; // clear offset list
    CalcOffsetKernel calcOffsetsKernel; // create new offset list

    // Other fluid sim kernels
    CalcDensitiesKernel densityKernel;
    CalcForcesKernel forceKernel;
    TimeIntegrateKernel integrateKernel;
};

}
