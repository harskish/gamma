#pragma once

#include <clt.hpp>
#include "ParticleSystem.hpp"
#include "SPHKernels.hpp"

class CameraBase;

namespace SPH {

class SPHSimulator : public ParticleSystem {
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
    void setupCL();
    void buildKernels();

    clt::State clState;

    FindNeighborsKernel neighborKernel;
    TimeIntegrateKernel integrateKernel;

};

}
