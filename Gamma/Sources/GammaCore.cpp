#include "GammaCore.hpp"
#include "GammaRenderer.hpp"
#include "GammaPhysics.hpp"
#include "gamma.hpp"
#include "utils.hpp"
#include "Model.hpp"
#include "OrbitCamera.hpp"
#include "FlightCamera.hpp"
#include "Simulation/SPH/SPH.hpp"
#include "Simulation/Particles/GravitySim.hpp"
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

    // Setup camera
    camera.reset(new FlightCamera(CameraType::PERSP, mWindow)); // new OrbitCamera()
    //camera->place(glm::vec3(2.0f, 3.0f, 0.0f), glm::vec3(-1.0f, -1.0f, 0.0f));
    camera->place(glm::vec3(0.0f, 0.0f, 1.5f), glm::vec3(0.0f, 0.0f, -1.0f));
    renderer->linkCamera(camera);

    // Setup scene
    scene.reset(new Scene());
    setupFluidScene();
    renderer->linkScene(scene);
    physics->linkScene(scene);
}

GammaCore::~GammaCore(void) {
    // Indirectly force release of GL objects before glfwTerminate()
    renderer.reset();
    physics.reset();
    scene.reset();
    camera.reset();
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
        //while (lagMs >= MS_PER_UPDATE) {
        //    physics->step();
        //    lagMs -= MS_PER_UPDATE;
        //}
        physics->step();

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
    scene->addLight(new DirectionalLight(vec3(0.0, 0.0, -1.0), vec3(3.0)));
    scene->loadIBLMaps("Gamma/Assets/IBL/pisa.hdr");
}

void GammaCore::setupPlane() {
    if (!scene) return;

    Mesh mesh = Mesh::Plane(3.0f, 3.0f);
    mesh.loadPBRTextures("Gamma/Assets/Textures/speckled");

    Model model(mesh);
    model.translate(0.0, -1.0, 0.0);
    scene->addModel(model);

    // Point lights (w=1)
    //scene->addLight(new PointLight(vec3(-0.5, 0.0, -0.5), vec3(5.0, 5.0, 5.0)));

    // Directional lights (w=0)
    scene->addLight(new DirectionalLight(vec3(-1.0, -1.0, 0.0), vec3(2.0, 2.0, 2.0)));
}

void GammaCore::setupShadowScene() {
    setupPlane();

    Model pot("Gamma/Assets/Models/teapot2.ply");
    pot.getMesh(0).loadPBRTextures("Gamma/Assets/Textures/bamboo");
    pot.translate(0.0f, -0.7f, 0.0f);
    pot.scale(0.3f);    
    scene->addModel(pot);

    scene->loadIBLMaps("Gamma/Assets/IBL/pisa.hdr");
}

void GammaCore::setupHelmetScene() {
	scene->initFromFile("Gamma/Assets/Models/helmet/helmet.gscn");
    scene->loadIBLMaps("Gamma/Assets/IBL/bloom_test.hdr");
}

void GammaCore::setupFluidScene() {
    scene->addParticleSystem(std::make_shared<SPH::SPHSimulator>());
    renderer->useFXAA = false;
}

void GammaCore::setupParticleScene() {
    scene->addParticleSystem(std::make_shared<Gravity::GravitySimulator>());
    renderer->useFXAA = false;
}

// Place a light based on the camera's position
void GammaCore::placeLight() {
    if (scene->lights().size() >= GammaRenderer::MAX_LIGHTS)
        scene->clearLights();
    
    if (ImGui::GetIO().KeyCtrl)
        scene->addLight(new DirectionalLight(-camera->getViewDirection(), vec3(3.0)));
    else
        scene->addLight(new PointLight(camera->getPosition(), vec3(3.0)));
}

// UI state
static bool showMetrics = false;
static bool showImguiDemo = false;
static bool showSettings = false;

// Main menu with scene loading etc.
void GammaCore::drawUI() {
    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Load", "L")) { openFileSelector(); }
            if (ImGui::MenuItem("Quit", "Esc")) { glfwSetWindowShouldClose(mWindow, true); }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Settings", NULL, &showSettings);
            ImGui::MenuItem("Metrics", NULL, &showMetrics);
            ImGui::MenuItem("Imgui demo", NULL, &showImguiDemo);
            ImGui::EndMenu();
        }

        auto reset = [&](void(GammaCore::*load)()) {
            scene.reset(new Scene());
            (this->*load)();
            renderer->linkScene(scene);
            physics->linkScene(scene);
        };

        if (ImGui::BeginMenu("Scenes")) {
            if (ImGui::MenuItem("Teapot scene", NULL)) {
                reset(&GammaCore::setupShadowScene);
            }
            if (ImGui::MenuItem("Helmet scene", NULL)) {
                reset(&GammaCore::setupHelmetScene);
            }
            if (ImGui::MenuItem("Sphere scene", NULL)) {
                reset(&GammaCore::setupSphereScene);
            }
            if (ImGui::MenuItem("Particle scene", NULL)) {
                reset(&GammaCore::setupParticleScene);
            }
            if (ImGui::MenuItem("Fluid scene", NULL)) {
                reset(&GammaCore::setupFluidScene);
            }
            ImGui::EndMenu();
        }
        ImGui::EndMainMenuBar();
    }

    if (showImguiDemo) ImGui::ShowDemoWindow(&showImguiDemo);
    if (showMetrics) renderer->drawStats(&showMetrics);
    if (showSettings) renderer->drawSettings(&showSettings);
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

// Functional keys that need to be triggered only once per press
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

    if (io.WantCaptureKeyboard || action == GLFW_RELEASE) return;

    #define match(key, expr) case key: expr; break;
    switch (key) {
        match(GLFW_KEY_SPACE, placeLight());
        match(GLFW_KEY_F5, GLProgram::clearCache()); // force recompile of shaders
        match(GLFW_KEY_ESCAPE, glfwSetWindowShouldClose(mWindow, true));
        match(GLFW_KEY_L, openFileSelector());
    }
    #undef match
}

// Instant and simultaneous key presses (movement etc.)
void GammaCore::pollKeys(float deltaT) {
    if (ImGui::GetIO().WantCaptureKeyboard) return;

    #define check(key, expr) if(glfwGetKey(mWindow, key) == GLFW_PRESS) { expr; }
    check(GLFW_KEY_W, camera->move(CameraMovement::FORWARD, deltaT));
    check(GLFW_KEY_S, camera->move(CameraMovement::BACKWARD, deltaT));
    check(GLFW_KEY_A, camera->move(CameraMovement::LEFT, deltaT));
    check(GLFW_KEY_D, camera->move(CameraMovement::RIGHT, deltaT));
    check(GLFW_KEY_R, camera->move(CameraMovement::UP, deltaT));
    check(GLFW_KEY_F, camera->move(CameraMovement::DOWN, deltaT));
    #undef check
}

void GammaCore::openFileSelector() {
    const char* message = "Select model or environment map";
    const char* defaultPath = "Gamma/Assets/Models";
    const std::vector<const char*> filter = { "*.gscn", "*.fbx", "*.dae", "*.obj", "*.3ds", "*.ply", "*.hdr" };

    char const *selected = tinyfd_openFileDialog(message, defaultPath, filter.size(), filter.data(), NULL, 0);
    
    // User didn't press cancel
    if (selected) {
        std::string path = unixifyPath(std::string(selected));
        try {
            if (endsWith(path, ".hdr")) {
                scene->loadIBLMaps(path);
            }
            else if (endsWith(path, ".gscn")) {
                scene->clearModels();
                scene->initFromFile(path.c_str());
            }
            else {
                Model m(path);
                m.normalizeScale();
                scene->clearModels();
                scene->addModel(m);
            }
        }
        catch (std::runtime_error e) {
            std::cout << "Could not load model '" << selected << "':" << e.what() << std::endl;
        }
    }
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

    // UI Scale
    const float UIscale = 1.0f;
    ImGui::GetIO().FontGlobalScale = UIscale;
    ImGui::GetStyle().ScaleAllSizes(UIscale);
}
