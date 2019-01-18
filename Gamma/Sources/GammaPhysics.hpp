#pragma once

#include <memory>
#include "Scene.hpp"

class GammaPhysics
{
public:
    GammaPhysics(const double stepSize) : MS_PER_UPDATE(stepSize) {};
    ~GammaPhysics(void) = default;

    void linkScene(std::shared_ptr<Scene> s) { scene = s; }

    // Step size is constant
    void step(void);

private:
    GammaPhysics(const GammaPhysics&) = delete;
    GammaPhysics& operator=(const GammaPhysics&) = delete;

    std::shared_ptr<Scene> scene; // shared with renderer

    const double MS_PER_UPDATE;
};
