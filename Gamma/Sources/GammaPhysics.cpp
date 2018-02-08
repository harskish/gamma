#include "GammaPhysics.hpp"
#include <chrono>
#include <thread>

void GammaPhysics::step(void) {
    // Simulate computation
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
}
