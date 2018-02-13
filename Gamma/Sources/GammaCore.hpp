#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include "Scene.hpp"

class GammaRenderer;
class GammaPhysics;

// Core engine that manages timing of rendering and physics
class GammaCore
{
public:
    GammaCore(void);
    ~GammaCore(void);

    void mainLoop();
    void reshape();
    
private:
    GammaCore(const GammaCore&) = delete;
    GammaCore& operator=(const GammaCore&) = delete;

    void initGL();

    std::unique_ptr<GammaRenderer> renderer;
    std::unique_ptr<GammaPhysics> physics;
    GLFWwindow *mWindow;

    // Scene data, shared between renderer and physics engine
    std::shared_ptr<Scene> scene;

    // Rendering and physics parameters
    const int PHYSICS_FPS = 75; // typically 60+
    const double MS_PER_UPDATE = 1000.0 / PHYSICS_FPS;

};
