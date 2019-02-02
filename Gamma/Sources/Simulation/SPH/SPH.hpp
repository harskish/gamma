#pragma once

#include <vexcl/vexcl.hpp>
#include <clt.hpp>
#include "ParticleSystem.hpp"
#include "SPHKernels.hpp"

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

private:
    void setup();
    void buildKernels();

    FindNeighborsKernel neighborKernel;
    CalcDensitiesKernel densityKernel;
    CalcForcesKernel forceKernel;
    TimeIntegrateKernel integrateKernel;

};

}
