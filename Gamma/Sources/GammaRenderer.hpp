#pragma once

class GammaRenderer
{
public:
    GammaRenderer(void) {};
    ~GammaRenderer(void) {};

    void render(void);

private:
    GammaRenderer(const GammaRenderer&) = delete;
    GammaRenderer& operator=(const GammaRenderer&) = delete;

};