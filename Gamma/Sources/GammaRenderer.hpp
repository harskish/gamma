#pragma once
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <imgui.h>
#include "Scene.hpp"
#include "Camera.hpp"

class GammaRenderer
{
public:
    GammaRenderer(GLFWwindow *w);
    ~GammaRenderer() = default;

    void linkScene(std::shared_ptr<Scene> scene);
    void linkCamera(std::shared_ptr<CameraBase> camera) { this->camera = camera; }
    void render();
    void drawStats(bool *show);
    void drawSettings(bool *show);
    void reshape();
    void setRenderScale(float s) { renderScale = s; reshape(); }

private:
    GammaRenderer(const GammaRenderer&) = delete;
    GammaRenderer& operator=(const GammaRenderer&) = delete;

    void shadowPass();
    void shadingPass();
    void postProcessPass();

    // Inserted into shaders as #define
    const size_t MAX_LIGHTS = 8;
    void createDefaultCubemap(GLProgram *prog);
    void setupFBO();

    // Rendering statistics
    // Double buffered to avoid waiting for results
    #define NUM_STATS 3 // shadows, shading, post-processing
    unsigned int queryID[2][NUM_STATS];
    unsigned int queryBackBuffer = 0, queryFrontBuffer = 1;
    void genQueryBuffers();
    void swapQueryBuffers();

    GLFWwindow *window;
    std::shared_ptr<Scene> scene;
    std::shared_ptr<CameraBase> camera;

    float renderScale = 1.0f; // scales the FB size
    int fbWidth, fbHeight;
    int windowWidth, windowHeight;


    // FBO for postprocessing
    GLuint fbo = 0;
    GLuint colorTex[2] = { 0, 0 }; // ping pong
    GLuint depthTex = 0;
    int colorDst = 0; // index into colorTex
};