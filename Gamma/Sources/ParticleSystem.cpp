#include "ParticleSystem.hpp"

clt::State GPUParticleSystem::clState;

GPUParticleSystem::GPUParticleSystem() {
    // Setup CL once
    if (!clState.device()) {
        std::cout << "Creating OpenCL context" << std::endl;
        setupCL();
    }
    else {
        std::cout << "Reusing OpenCL context" << std::endl;
    }
}

void GPUParticleSystem::setupCL() {
    clState = clt::initialize("NVIDIA", "750");
    if (!clState.hasGLInterop)
        throw std::runtime_error("[GPUParticleSystem] Could not initialize CL-GL sharing");

    clt::setCpuDebug(false);
    clt::setKernelCacheDir("Gamma/Cache/kernel_binaries");
}
