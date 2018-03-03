#include "GammaCore.hpp"
#include "GammaRenderer.hpp"
#include "GammaPhysics.hpp"
#include "gamma.hpp"
#include "utils.hpp"
#include "Model.hpp"
#include "OrbitCamera.hpp"
#include "FlightCamera.hpp"
#include <tinyfiledialogs.h>
#include <imgui.h>
#include "imgui_impl_glfw_gl3.h"

using glm::vec4;
using glm::vec3;
using glm::vec2;

GammaCore::GammaCore(void) {
    // Setup renderer and physics engine
    initGL();
    initImgui();
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
        ImGui_ImplGlfwGL3_NewFrame();
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
        drawUI();

        // Flip buffers and draw
        ImGui::Render();
        ImGui_ImplGlfwGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(mWindow);
        glfwPollEvents();
    }
}

void GammaCore::setupSphereScene() {
    if (!scene) return;
    
    Model obj("Gamma/Assets/Models/sphere.obj");

    vector<string> texPaths = {
        "Gamma/Assets/Textures/bamboo",
        "Gamma/Assets/Textures/cement",
        "Gamma/Assets/Textures/gold",
        "Gamma/Assets/Textures/rusty",
        "Gamma/Assets/Textures/speckled",
        "Gamma/Assets/Textures/rubber",
        "Gamma/Assets/Textures/plastic",
        "Gamma/Assets/Textures/limestone",
        "Gamma/Assets/Textures/pan1",
        "Gamma/Assets/Textures/granite",
        "Gamma/Assets/Textures/pan2",
        "Gamma/Assets/Textures/mahogany",
        "Gamma/Assets/Textures/streaked",
    };

    const int rows = 3;
    const int cols = 3;
    const float dr = 1.0f / (float)(rows - 1);
    const float dc = 1.0f / (float)(cols - 1);

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            // Create clone with its own materials and transforms
            Model instance = obj;
            std::string tex = texPaths[(r * cols + c) % (texPaths.size() - 1)];
            instance.getMesh(0).loadPBRTextures(tex);
            
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
    scene->addLight(Light(vec4(0.0, 3.0, 30.0, 1.0), vec3(2000.0)));
}

void GammaCore::setupPlane() {
    if (!scene) return;

    Mesh mesh = Mesh::Plane(1.0f, 1.0f);
    mesh.loadPBRTextures("Gamma/Assets/Textures/gold");

    Model model(mesh);
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

// UI state
static bool showMetrics = false;
static bool showImguiDemo = false;

// Main menu with scene loading etc.
void GammaCore::drawUI() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Load", "L")) { openModelSelector(); }
            if (ImGui::MenuItem("Quit", "Esc")) { glfwSetWindowShouldClose(mWindow, true); }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Metrics", NULL, &showMetrics);
            ImGui::MenuItem("Imgui demo", NULL, &showImguiDemo);
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (showImguiDemo) ImGui::ShowDemoWindow(&showImguiDemo);
    if (showMetrics) renderer->drawStats(&showMetrics);
}

void GammaCore::reshape() {
    renderer->reshape();
    camera->viewportUpdate();
}

void GammaCore::handleMouseButton(int key, int action, int mods) {
    if (ImGui::GetIO().WantCaptureMouse) return;
    camera->handleMouseButton(key, action);
}

void GammaCore::handleCursorPos(double x, double y) {
    camera->handleMouseCursor(x, y);
}

void GammaCore::handleFileDrop(int count, const char ** filenames) {
    std::cout << count << " files dropped onto window" << std::endl;
}

void GammaCore::handleMouseScroll(double deltaX, double deltaY) {
    ImGuiIO& io = ImGui::GetIO();
    io.MouseWheelH += (float)deltaX;
    io.MouseWheel += (float)deltaY;
    if (io.WantCaptureMouse) return;

    camera->handleMouseScroll(deltaX, deltaY);
}

void GammaCore::handleChar(unsigned int c) {
    ImGuiIO& io = ImGui::GetIO();
    if (c > 0 && c < 0x10000)
        io.AddInputCharacter((unsigned short)c);
}

void GammaCore::handleKey(int key, int action, int mods) {
    ImGuiIO& io = ImGui::GetIO();
    if (action == GLFW_PRESS)
        io.KeysDown[key] = true;
    if (action == GLFW_RELEASE)
        io.KeysDown[key] = false;

    (void)mods; // Modifiers are not reliable across systems
    io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
    io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
    io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
    io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
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
    if (ImGui::GetIO().WantCaptureKeyboard) return;

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

void charCallback(GLFWwindow* window, unsigned int c) {
    corePointer(window)->handleChar(c);
}

void keyCallback(GLFWwindow* window, int key, int, int action, int mods) {
    corePointer(window)->handleKey(key, action, mods);
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
    fprintf(stdout, "Vendor: %s\n", glGetString(GL_VENDOR));

    glfwSetWindowUserPointer(mWindow, this);
    glfwSetWindowSizeCallback(mWindow, windowSizeCallback);
    glfwSetCursorPosCallback(mWindow, cursorPositionCallback);
    glfwSetMouseButtonCallback(mWindow, mouseButtonCallback);
    glfwSetDropCallback(mWindow, dropCallback);
    glfwSetScrollCallback(mWindow, scrollCallback);
    glfwSetCharCallback(mWindow, charCallback);
    glfwSetKeyCallback(mWindow, keyCallback);
}

void GammaCore::initImgui() {
    ImGui::CreateContext();
    ImGui_ImplGlfwGL3_Init(mWindow, false); // use own callbacks
    ImGui::StyleColorsDark();

    // Scale
    //int ww, wh, fw, fh;
    //glfwGetWindowSize(mWindow, &ww, &wh);
    //glfwGetFramebufferSize(mWindow, &fw, &fh);*/
    //const float scale = glm::min((float)fw/ww, (float)fh/wh);
    const float scale = 1.5f;
    ImGui::GetIO().FontGlobalScale = scale;
    ImGui::GetStyle().ScaleAllSizes(scale);
}
