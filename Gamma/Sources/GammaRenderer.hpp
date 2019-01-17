#pragma once
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <imgui.h>
#include "Scene.hpp"
#include "Camera.hpp"
#include "IBLMaps.hpp"
#include "BloomPass.hpp"

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

    // Inserted into shaders as #define
    static const size_t MAX_LIGHTS = 6;

    bool useFXAA = true;
    bool useBloom = true;

private:
    GammaRenderer(const GammaRenderer&) = delete;
    GammaRenderer& operator=(const GammaRenderer&) = delete;

    void shadowPass();
    void shadingPass();
    void postProcessPass();
    
    void drawSkybox();

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
    std::unique_ptr<BloomPass> bloomPass;
    int tonemapOp = 0; // 0 => Uncharted 2, 1 => Reinhard
    float tonemapExposure = 1.0f;

    // FBO for postprocessing
    GLuint fbo = 0;
    GLuint colorTex[2] = { 0, 0 }; // ping pong
    GLuint depthTex = 0;
    int colorDst = 0; // index into colorTex
};
