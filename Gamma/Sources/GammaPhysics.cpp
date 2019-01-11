#include "GammaPhysics.hpp"
#include "Scene.hpp"
#include <chrono>
#include <thread>

void GammaPhysics::step(void) {
    for (auto& sys : scene->particleSystems()) {
        sys->update();
    }
}
