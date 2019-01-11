#pragma once
#include "GLWrappers.hpp"
#include "GLProgram.hpp"

class CameraBase;

class ParticleSystem {
public:
    virtual void update() {}
    virtual void render(const CameraBase* camera) {}
    virtual void renderUnshaded() {}

protected:
    // TODO: remove shared_ptr, implement VBO/VAO copy constructors
    std::shared_ptr<VertexBuffer> positions;
    std::shared_ptr<VertexBuffer> velocities;
    std::shared_ptr<VertexArray> particles; // todo: needed?
};
