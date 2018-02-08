#pragma once

class GammaPhysics
{
public:
    GammaPhysics(const double stepSize) : MS_PER_UPDATE(stepSize) {};
    ~GammaPhysics(void) = default;

    // Step size is constant
    void step(void);

private:
    GammaPhysics(const GammaPhysics&) = delete;
    GammaPhysics& operator=(const GammaPhysics&) = delete;

    const double MS_PER_UPDATE;

};
