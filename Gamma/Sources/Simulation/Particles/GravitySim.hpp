#pragma once

#include <clt.hpp>
#include "ParticleSystem.hpp"
#include "GravitySimKernels.hpp"

class CameraBase;

namespace Gravity {

class GravitySimulator : public ParticleSystem {
public:
    GravitySimulator(void);
    ~GravitySimulator(void) {
        std::cout << "[GravitySimulator] Shutting down" << std::endl;
    }

    void update() override;
    void render(const CameraBase* camera) override;
    void renderUnshaded() override;

private:
    void setup();
    void setupCL();
    void buildKernels();

    clt::State clState;

    TimeIntegrateKernel integrateKernel;

};

}
