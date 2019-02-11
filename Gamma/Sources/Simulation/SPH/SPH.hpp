#pragma once

#include "ParticleSystem.hpp"
#include "SPHKernels.hpp"
#include <clt.hpp>
#include <vexcl/vexcl.hpp>
#include <memory>
#include <vector>

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
    std::vector<clt::Kernel*> getKernels(); // for building/rebuilding all

    // Hash grid
    CalcCellIndexKernel cellIdxKernel; // flat cell indices, before sort
    ClearOffsetKernel clearOffsetsKernel; // clear offset list
    CalcOffsetKernel calcOffsetsKernel; // create new offset list

    // Core SPH kernels
    CalcDensitiesKernel densityKernel;
    CalcForcesKernel forceKernel;

    // Integrators
    EulerIntegrateKernel eulerKernel;
    LeapfrogKernel leapfrogKernel;
    LeapfrogStartKernel leapfrogStartKernel;

    cl_uint iteration = 0;
};

}
