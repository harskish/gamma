#include "GammaCore.hpp"
#include "GammaRenderer.hpp"
#include "GammaPhysics.hpp"
#include "gamma.hpp"
#include "utils.hpp"
#include "Model.hpp"
#include "OrbitCamera.hpp"

GammaCore::GammaCore(void) {
    // Setup renderer and physics engine
    initGL();
    renderer.reset(new GammaRenderer(mWindow));
    physics.reset(new GammaPhysics(MS_PER_UPDATE));

    // Setup scene
    scene.reset(new Scene());
    //scene->addModel(Model("Gamma/Assets/Models/teapot.ply").translate(0.0f, 2.25f, 0.0f));
    scene->addModel(Model("Gamma/Assets/Models/nanosuit/nanosuit.obj").scale(0.3f).translate(0.0f, -8.0f, 0.0f));
    scene->addModel(Model("Gamma/Assets/Models/apple/apple.obj").scale(0.6f).translate(0.0f, 2.5f, 0.0f));
    renderer->linkScene(scene);

    camera.reset(new OrbitCamera(CameraType::PERSP, mWindow));
    renderer->linkCamera(camera);
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

void GammaCore::reshape() {
    renderer->reshape();
    camera->viewportUpdate();
}

void GammaCore::handleMouseButton(int key, int action, int mods) {
    camera->handleMouseButton(key, action);
}

void GammaCore::handleCursorPos(double x, double y) {
    camera->handleMouseCursor(x, y);
}

void GammaCore::handleFileDrop(int count, const char ** filenames) {
    std::cout << count << " files dropped onto window" << std::endl;
}

void GammaCore::handleMouseScroll(double deltaX, double deltaY) {
    camera->handleMouseScroll(deltaX, deltaY);
}

inline GammaCore *corePointer(GLFWwindow* window) {
    return static_cast<GammaCore*>(glfwGetWindowUserPointer(window));
}

void windowSizeCallback(GLFWwindow* window, int, int) {
    corePointer(window)->reshape();
}

void cursorPositionCallback(GLFWwindow* window, double xpos, double ypos) {
    corePointer(window)->handleCursorPos(xpos, ypos);
}

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    corePointer(window)->handleMouseButton(button, action, mods);
}

void dropCallback(GLFWwindow *window, int count, const char **filenames) {
    corePointer(window)->handleFileDrop(count, filenames);
}

void scrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    corePointer(window)->handleMouseScroll(xoffset, yoffset);
}

void GammaCore::initGL() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);
    mWindow = glfwCreateWindow(mWidth, mHeight, "Gamma", nullptr, nullptr);

    // Check for Valid Context
    if (mWindow == nullptr) {
        fprintf(stdout, "Failed to Create OpenGL Context");
        throw std::runtime_error("OpenGL context creation failed");
    }

    // Create Context and Load OpenGL Functions
    glfwMakeContextCurrent(mWindow);
    gladLoadGL();
    glCheckError();
    fprintf(stdout, "OpenGL %s\n", glGetString(GL_VERSION));

    glfwSetWindowUserPointer(mWindow, this);
    glfwSetWindowSizeCallback(mWindow, windowSizeCallback);
    glfwSetCursorPosCallback(mWindow, cursorPositionCallback);
    glfwSetMouseButtonCallback(mWindow, mouseButtonCallback);
    glfwSetDropCallback(mWindow, dropCallback);
    glfwSetScrollCallback(mWindow, scrollCallback);
}
