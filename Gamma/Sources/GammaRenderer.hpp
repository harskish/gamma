#pragma once
#include <GLFW/glfw3.h>

class GammaRenderer
{
public:
    GammaRenderer(GLFWwindow *w) : window(w) {};
    ~GammaRenderer(void) {};

    void render(void);

private:
    GammaRenderer(const GammaRenderer&) = delete;
    GammaRenderer& operator=(const GammaRenderer&) = delete;

    GLFWwindow *window;

};