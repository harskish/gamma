#include "GammaCore.hpp"
#include "GammaRenderer.hpp"
#include "GammaPhysics.hpp"
#include "gamma.hpp"
#include "utils.hpp"

GammaCore::GammaCore(void) {
    // Setup renderer and physics engine
    initGL();
    renderer.reset(new GammaRenderer(mWindow));
    physics.reset(new GammaPhysics(MS_PER_UPDATE));
}

GammaCore::~GammaCore(void) {
    glfwTerminate();
    std::cout << "Core engine shutdown" << std::endl;
}

// Run physics at constant pace, render whenever possible
// More info: http://gameprogrammingpatterns.com/game-loop.html
void GammaCore::mainLoop() {
    double lastTime = glfwGetTime();
    double lagMs = 0.0; // how much simulation lags behind world clock
    while (!glfwWindowShouldClose(mWindow)) {
        double current = glfwGetTime();
        double deltaT = current - lastTime;
        lastTime = current;
        lagMs += 1000.0 * deltaT;

        // Process input
        if (glfwGetKey(mWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(mWindow, true);

        // Catch up to real world clock
        while (lagMs >= MS_PER_UPDATE) {
            physics->step();
            lagMs -= MS_PER_UPDATE;
        }

        // Render
        renderer->render();

        // Flip Buffers and Draw
        glfwSwapBuffers(mWindow);
        glfwPollEvents();
    }
}

void GammaCore::initGL(void) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
    mWindow = glfwCreateWindow(mWidth, mHeight, "Gamma", nullptr, nullptr);

    // Check for Valid Context
    if (mWindow == nullptr) {
        fprintf(stdout, "Failed to Create OpenGL Context");
        throw std::runtime_error("Shader compilation failed!");
    }

    // Create Context and Load OpenGL Functions
    glfwMakeContextCurrent(mWindow);
    gladLoadGL();
    glCheckError();
    fprintf(stdout, "OpenGL %s\n", glGetString(GL_VERSION));
}
