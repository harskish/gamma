#include "GammaCore.hpp"
#include "GammaRenderer.hpp"
#include "GammaPhysics.hpp"
#include "gamma.hpp"
#include "utils.hpp"
#include "Model.hpp"
#include "OrbitCamera.hpp"
#include "FlightCamera.hpp"
#include <tinyfiledialogs.h>

GammaCore::GammaCore(void) {
    // Setup renderer and physics engine
    initGL();
    renderer.reset(new GammaRenderer(mWindow));
    physics.reset(new GammaPhysics(MS_PER_UPDATE));

    // Setup scene
    scene.reset(new Scene());
    setupSphereScene();
    renderer->linkScene(scene);

    //camera.reset(new OrbitCamera(CameraType::PERSP, mWindow));
    camera.reset(new FlightCamera(CameraType::PERSP, mWindow));
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
        pollKeys(deltaT);

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

void GammaCore::setupSphereScene() {
    if (!scene) return;
    
    Model obj("Gamma/Assets/Models/sphere.obj");

    const int rows = 7;
    const int cols = 7;
    const float dr = 1.0f / (float)(rows - 1);
    const float dc = 1.0f / (float)(cols - 1);

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            // Create clone with its own materials and transforms
            Model instance = obj;
            
            // Vary smoothness and metallic
            for (Material* m : instance.getMaterials()) {
                m->alpha = r * dr + 0.05f;
                m->metallic = c * dc + 0.05f;
                m->Kd = glm::vec3(0.5f, 0.05f, 0.05f);
            }

            // Vary position
            float posX = -1.0f + 2.0f * r * dr;
            float posY = -1.0f + 2.0f * c * dc;
            instance.setXform(glm::mat4(1.0f));
            instance.translate(posX, posY, 0.0f).scale(0.8f / glm::max(rows, cols));

            // Add instance to scene
            scene->addModel(instance);
        }
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

void GammaCore::openModelSelector() {
    const std::string message = "Select model to load";
    const std::string defaultPath = "Gamma/Assets/Models";
    const std::vector<const char*> filter = { "" };

    char const *selected = tinyfd_openFileDialog(message.c_str(), defaultPath.c_str(), filter.size(), filter.data(), NULL, 0);
    
    // User didn't press cancel
    if (selected) {
        try {
            std::string path = unixifyPath(std::string(selected));
            Model m(path);
            scene->clearModels();
            scene->addModel(m);
        }
        catch (std::runtime_error e) {
            std::cout << "Could not load model '" << selected << "':" << e.what() << std::endl;
        }
    }
}

#define check(key, expr) if(glfwGetKey(mWindow, key) == GLFW_PRESS) { expr; }
void GammaCore::pollKeys(float deltaT) {
    check(GLFW_KEY_ESCAPE, glfwSetWindowShouldClose(mWindow, true));
    check(GLFW_KEY_L, openModelSelector());
    check(GLFW_KEY_W, camera->move(CameraMovement::FORWARD, deltaT));
    check(GLFW_KEY_S, camera->move(CameraMovement::BACKWARD, deltaT));
    check(GLFW_KEY_A, camera->move(CameraMovement::LEFT, deltaT));
    check(GLFW_KEY_D, camera->move(CameraMovement::RIGHT, deltaT));
    check(GLFW_KEY_R, camera->move(CameraMovement::UP, deltaT));
    check(GLFW_KEY_F, camera->move(CameraMovement::DOWN, deltaT));
}
#undef check

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
