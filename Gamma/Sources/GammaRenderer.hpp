#pragma once
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include "Scene.hpp"

class GammaRenderer
{
public:
    GammaRenderer(GLFWwindow *w);
    ~GammaRenderer() = default;

    void linkScene(std::shared_ptr<Scene> scene) { this->scene = scene; }
    void render();
    void reshape();

private:
    GammaRenderer(const GammaRenderer&) = delete;
    GammaRenderer& operator=(const GammaRenderer&) = delete;

    GLFWwindow *window;
    std::shared_ptr<Scene> scene;
};