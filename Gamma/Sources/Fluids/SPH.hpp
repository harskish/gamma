#pragma once

#include <clt.hpp>
#include "ParticleSystem.hpp"
#include "SPHKernels.hpp"
#include "GLWrappers.hpp"
#include "Camera.hpp"

namespace SPH {

class SPHSimulator : public ParticleSystem {
public:
    SPHSimulator(void);

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
