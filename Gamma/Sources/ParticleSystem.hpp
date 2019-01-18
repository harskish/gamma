#pragma once
#include "GLWrappers.hpp"
#include <clt.hpp>

class CameraBase;

class ParticleSystem {
public:
    virtual void update() {}
    virtual void render(const CameraBase*) { }
    virtual void renderUnshaded() {}

protected:
    // TODO: remove shared_ptr, implement VBO/VAO copy constructors
    std::shared_ptr<VertexBuffer> positions;
    std::shared_ptr<VertexBuffer> velocities;
    std::shared_ptr<VertexArray> particles; // todo: needed?
};

class GPUParticleSystem : public ParticleSystem {
protected:
    GPUParticleSystem();
    static clt::State clState; // one instance globally

private:
    void setupCL(); // should not be called externally
};
