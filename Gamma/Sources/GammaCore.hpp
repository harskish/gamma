#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include "Scene.hpp"
#include "Camera.hpp"

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
    void handleMouseButton(int key, int action, int mods);
    void handleCursorPos(double x, double y);
    void handleFileDrop(int count, const char **filenames);
    void handleMouseScroll(double deltaX, double deltaY);

    void openModelSelector();

    // Test scenes
    void setupSphereScene();
    
private:
    GammaCore(const GammaCore&) = delete;
    GammaCore& operator=(const GammaCore&) = delete;

    void initGL();

    std::unique_ptr<GammaRenderer> renderer;
    std::unique_ptr<GammaPhysics> physics;
    GLFWwindow *mWindow;

    // Data shared between renderer and physics engine
    std::shared_ptr<Scene> scene;
    std::shared_ptr<CameraBase> camera;

    // Rendering and physics parameters
    const int PHYSICS_FPS = 75; // typically 60+
    const double MS_PER_UPDATE = 1000.0 / PHYSICS_FPS;

};
