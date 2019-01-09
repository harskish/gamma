#pragma once
#include "GLWrappers.hpp"

class ParticleSystem {
public:
    GLuint getPosBuffer() { return positions.id; }
    GLuint getVelBuffer() { return velocities.id; }

    virtual void update() {};
    virtual void render() {};

protected:
    VertexBuffer positions;
    VertexBuffer velocities;
    VertexArray particles; // todo: needed?
};
