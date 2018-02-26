#pragma once
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include "Scene.hpp"
#include "Camera.hpp"

class GammaRenderer
{
public:
    GammaRenderer(GLFWwindow *w);
    ~GammaRenderer() = default;

    void linkScene(std::shared_ptr<Scene> scene) { this->scene = scene; }
    void linkCamera(std::shared_ptr<CameraBase> camera) { this->camera = camera; }
    void render();
    void reshape();

private:
    GammaRenderer(const GammaRenderer&) = delete;
    GammaRenderer& operator=(const GammaRenderer&) = delete;

    // Inserted into shaders as #define
    const unsigned int MAX_LIGHTS = 16;

    GLFWwindow *window;
    std::shared_ptr<Scene> scene;
    std::shared_ptr<CameraBase> camera;
};