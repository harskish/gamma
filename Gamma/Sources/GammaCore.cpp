#include "GammaCore.hpp"
#include "GammaRenderer.hpp"
#include "GammaPhysics.hpp"
#include "gamma.hpp"
#include "utils.hpp"
#include "Model.hpp"
#include "OrbitCamera.hpp"
#include "FlightCamera.hpp"
#include <tinyfiledialogs.h>

using glm::vec4;
using glm::vec3;
using glm::vec2;

GammaCore::GammaCore(void) {
    // Setup renderer and physics engine
    initGL();
    renderer.reset(new GammaRenderer(mWindow));
    physics.reset(new GammaPhysics(MS_PER_UPDATE));

    // Setup scene
    scene.reset(new Scene());
    //setupSphereScene();
    setupPlane();
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
    Mesh& m = obj.getMesh(0);
    Material &mat = m.getMaterial();

    std::vector<shared_ptr<Texture>> textures;
    auto add = [&](std::string path, TextureMask mask) {
        shared_ptr<Texture> tex = std::make_shared<Texture>();
        tex->id = textureFromFile(path);
        tex->type = mask;
        textures.push_back(tex);
    };

    add("Gamma/Assets/Textures/rusty/albedo.png", TextureMask::DIFFUSE);
    add("Gamma/Assets/Textures/rusty/metallic.png", TextureMask::METALLIC);
    add("Gamma/Assets/Textures/rusty/roughness.png", TextureMask::ROUGHNESS);
    add("Gamma/Assets/Textures/rusty/normal.png", TextureMask::NORMAL);
    m.setTextures(textures);

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
                m->alpha = glm::clamp(r * dr, 0.05f, 1.0f);
                m->metallic = c * dc;
                m->Kd = glm::vec3(0.4f, 0.0f, 0.0f);
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

    // Lights
    glm::vec3 e(50.0);
    scene->addLight(Light(vec4(1.0, 1.0, 10.0, 1.0), e));
    scene->addLight(Light(vec4(1.0, -1.0, 10.0, 1.0), e));
    scene->addLight(Light(vec4(-1.0, 1.0, 10.0, 1.0), e));
    scene->addLight(Light(vec4(-1.0, -1.0, 10.0, 1.0), e));
}

void GammaCore::setupPlane() {
    if (!scene) return;

    Vertex ur = { vec3(1.0, 0.0, 1.0), vec3(0.0, 1.0, 0.0), vec2(0.0, 1.0) };
    Vertex ul = { vec3(-1.0, 0.0, 1.0), vec3(0.0, 1.0, 0.0), vec2(0.0, 0.0) };
    Vertex br = { vec3(1.0, 0.0, -1.0), vec3(0.0, 1.0, 0.0), vec2(1.0, 1.0) };
    Vertex bl = { vec3(-1.0, 0.0, -1.0), vec3(0.0, 1.0, 0.0), vec2(1.0, 0.0) };
    
    std::vector<Vertex> verts = { ur, ul, br, bl };
    std::vector<unsigned int> inds = { 0, 1, 2, 2, 1, 3 };
    std::vector<shared_ptr<Texture>> textures;

    auto add = [&](std::string path, TextureMask mask) {
        shared_ptr<Texture> tex = std::make_shared<Texture>();
        tex->id = textureFromFile(path);
        tex->type = mask;
        textures.push_back(tex);
    };

    add("Gamma/Assets/Textures/rusty/albedo.png", TextureMask::DIFFUSE);
    add("Gamma/Assets/Textures/rusty/roughness.png", TextureMask::ROUGHNESS);
    add("Gamma/Assets/Textures/rusty/normal.png", TextureMask::NORMAL);
    add("Gamma/Assets/Textures/rusty/metallic.png", TextureMask::METALLIC);

    Material mat;
    mat.metallic = 0.0f;

    Model model(Mesh(verts, inds, textures, mat));
    model.translate(0.0, -1.0, 0.0);
    scene->addModel(model);

    // Point lights (w=1)
    scene->addLight(Light(vec4(-0.5, 0.0, -0.5, 1.0), vec3(5.0, 5.0, 5.0)));

    // Directional lights (w=0)
    //scene->addLight(Light(vec4(-1.0, -1.0, 0.0, 0.0), vec3(2.0, 2.0, 0.0)));
}

// Place a point light at the camera's position
void GammaCore::placeLight() {
    scene->clearLights();
    scene->addLight(Light(vec4(camera->getPosition(), 1.0), vec3(3.0)));
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
            m.normalizeScale();
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
    check(GLFW_KEY_SPACE, placeLight());
    check(GLFW_KEY_F5, GLProgram::clearCache()); // force recompile of shaders
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
