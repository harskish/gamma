#include "GammaRenderer.hpp"
#include "GLProgram.hpp"
#include "utils.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <map>

void GammaRenderer::linkScene(std::shared_ptr<Scene> scene) {
    this->scene = scene;
    scene->setMaxLights(MAX_LIGHTS);
}

void GammaRenderer::render() {
    // Draw (and filter) shadow maps
    shadowPass();

    // Perform shading
    shadingPass();

    // Postprocessing
    postProcessPass();
    
    // Debugging
    auto l = scene->lights()[0];
    GLuint dirLightTex = l->getTexHandle();
    if (dirLightTex > 0 && l->getVector().w == 0.0f) {
        showDepthTex(dirLightTex, 4, 4, 3);
    }

    glCheckError();
}

GammaRenderer::GammaRenderer(GLFWwindow * w) {
    this->window = w;
    this->scene.reset(new Scene()); // default empty scene

    glDisable(GL_CULL_FACE);
    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);

    genQueryBuffers();
    reshape(); // sets up FBO
}

void GammaRenderer::reshape() {
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    
    // Create new textures
    std::cout << "New FB size: " << fbWidth << "x" << fbWidth << std::endl;
    setupFBO();
}

void GammaRenderer::shadowPass() {
    // Render shadow maps
    for (Light *l : scene->lights()) {
        l->renderShadowMap(scene);
    }

    // Blur SVM shadows
    if (Light::useVSM) {
        for (Light *l : scene->lights()) {
            l->processShadowMap();
        }
    }
}

void GammaRenderer::shadingPass() {
    std::string progId = "Render::shadeGGX";
    GLProgram* prog = GLProgram::get(progId);
    if (!prog) {
        std::cout << "Compiling GGX program" << std::endl;
        std::map<std::string, std::string> repl;
        repl["$MAX_LIGHTS"] = "#define MAX_LIGHTS " + std::to_string(MAX_LIGHTS);
        prog = new GLProgram(readShader("Gamma/Shaders/ggx.vert", repl),
            readShader("Gamma/Shaders/ggx.frag", repl));
        GLProgram::set(progId, prog);
        createDefaultCubemap(prog);
    }

    // Draw into framebuffer for later post-processing
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex[0], 0);
    glViewport(0, 0, fbWidth, fbHeight);
    glClearColor(0.125f, 0.125f, 0.125f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Setup view-projection transform
    glm::mat4 P = camera->getP();
    glm::mat4 V = camera->getV();

    prog->use();
    prog->setUniform("V", V);
    prog->setUniform("P", P);
    prog->setUniform("cameraPos", camera->getPosition());
    glCheckError();

    // Set lights
    std::vector<Light*> &lights = scene->lights();
    prog->setUniform("nLights", (unsigned int)lights.size());
    for (size_t i = 0; i < lights.size(); i++) {
        Light *l = lights[i];
        prog->setUniform("lightVectors[" + std::to_string(i) + "]", l->vector);
        prog->setUniform("emissions[" + std::to_string(i) + "]", l->emission);
        prog->setUniform("lightTransforms[" + std::to_string(i) + "]", l->getLightTransform());

        if (l->getVector().w == 0.0) {
            glActiveTexture(GL_TEXTURE8 + i);
            prog->setUniform("shadowMaps[" + std::to_string(i) + "]", (int)(8 + i));
            glBindTexture(GL_TEXTURE_2D, l->getTexHandle());
        }
        else {
            glActiveTexture(GL_TEXTURE8 + MAX_LIGHTS + i);
            prog->setUniform("shadowCubeMaps[" + std::to_string(i) + "]", (int)(8 + MAX_LIGHTS + i));
            glBindTexture(GL_TEXTURE_CUBE_MAP, l->getTexHandle());
        }
        glCheckError();
    }
    glCheckError();

    // Setup texture locations (these are static)
    prog->setUniform("albedoMap", 0);
    prog->setUniform("normalMap", 1);
    prog->setUniform("shininessMap", 2);
    prog->setUniform("metallicMap", 3);

    // Setup other parameters
    prog->setUniform("useVSM", Light::useVSM);
    prog->setUniform("svmBleedFix", Light::svmBleedFix);

    // M set by each model before drawing
    glBeginQuery(GL_TIME_ELAPSED, queryID[queryBackBuffer][0]);
    glCheckError();
    for (Model &m : scene->models()) {
        m.render(prog);
    }
    glEndQuery(GL_TIME_ELAPSED);
    glCheckError();
}

void GammaRenderer::postProcessPass() {
    std::string progId = "Render::splat";
    GLProgram* prog = GLProgram::get(progId);
    if (!prog) {
        prog = new GLProgram(readShader("Gamma/Shaders/draw_tex_2d.vert"),
                             readShader("Gamma/Shaders/filter_null.frag"));
        GLProgram::set(progId, prog);
    }

    prog->use();
    applyFilter(prog, colorTex[0], 0, 0); // to screen
    glCheckError();
}

// Creates a dummy cubemap, fills array in shader. Will otherwise default to 0, 
// which is likely bound to a 2D texture, resultling in GL_INVALID_OPERATION.
void GammaRenderer::createDefaultCubemap(GLProgram *prog) {
    static GLuint shadowMap = 0;
    glDeleteTextures(1, &shadowMap);
    glGenTextures(1, &shadowMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, shadowMap);
    prog->use();

    for (size_t i = 0; i < MAX_LIGHTS; i++) {
        glActiveTexture(GL_TEXTURE8 + MAX_LIGHTS + i);
        prog->setUniform("shadowCubeMaps[" + std::to_string(i) + "]", (int)(8 + MAX_LIGHTS + i));
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowMap);
        glCheckError();
    }
}

// Used in post-processing
void GammaRenderer::setupFBO() {
    // Delete old objects in case of resize
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(2, colorTex);
    glDeleteTextures(1, &depthTex);

    // Generate new objects
    glGenFramebuffers(1, &fbo);
    glGenTextures(2, colorTex);
    glGenTextures(1, &depthTex);

    // Create depth texture
    glBindTexture(GL_TEXTURE_2D, depthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
        fbWidth, fbHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    // Color textures
    for (int i = 0; i < 2; i++) {
        glBindTexture(GL_TEXTURE_2D, colorTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F,
            fbWidth, fbHeight, 0, GL_RGBA, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }
    
    // Finalize framebuffer
    colorDst = 0;
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTex[colorDst], 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTex, 0);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();
}

void GammaRenderer::genQueryBuffers() {
    glGenQueries(NUM_STATS, queryID[queryBackBuffer]);
    glGenQueries(NUM_STATS, queryID[queryFrontBuffer]);
    glCheckError();
}

void GammaRenderer::swapQueryBuffers() {
    queryBackBuffer = 1U - queryBackBuffer;
    queryFrontBuffer = 1U - queryFrontBuffer;
}

void GammaRenderer::drawStats(bool *show) {
    // Early out if the window is collapsed
    if (!ImGui::Begin("Renderer stats", show)) {
        ImGui::End();
        return;
    }

    static float values[90] = { 0 };
    static int offs = 0;
    static bool firstFrame = true;

    GLuint64 timeNs = 1;
    if (!firstFrame) glGetQueryObjectui64v(queryID[queryFrontBuffer][0], GL_QUERY_RESULT, &timeNs);
    glCheckError();
    swapQueryBuffers();

    // Calculate average (last N samples)
    const int N = 20;
    float sum = 0.0f;
    for (int i = 0; i < N; i++) {
        sum += values[90 - N + i];
    }

    double timeMs = timeNs / 1e6;
    double timeMsAvg = sum / N;

    values[offs] = (float)timeMs;
    offs = (offs + 1) % IM_ARRAYSIZE(values);
    
    char label[20];
    sprintf(label, "Avg: %.2fms", timeMsAvg);
    ImGui::PlotLines(label, values, IM_ARRAYSIZE(values), offs, "Frametime (ms)", 0.0f, 10.0f, ImVec2(0, 80));

    firstFrame = false;
    ImGui::End();
}

// General renderer settings window
void GammaRenderer::drawSettings(bool * show) {
    if (!ImGui::Begin("Renderer settings", show)) {
        ImGui::End();
        return;
    }

    if (ImGui::CollapsingHeader("Lights")) {
        const char* items[] = { "128", "256", "512", "1024", "2048" ,"4096" };
        static int current_item_2_idx = 3;
        if (ImGui::Combo("Shadow resolution", &current_item_2_idx, items, IM_ARRAYSIZE(items))) {
            Light::defaultRes = 2 << (current_item_2_idx + 6);
            for (Light* l : scene->lights()) {
                l->initShadowMap();
            }
        }
        
        if (ImGui::Checkbox("Use VSM", &Light::useVSM)) {
            for (Light* l : scene->lights()) {
                l->initShadowMap();
            }
        }

        ImGui::SliderFloat("SVM anti-bleed", &Light::svmBleedFix, 0.0f, 0.9f);
        ImGui::SliderFloat("SVM blur size", &Light::svmBlur, 0.0f, 10.0f);
    }

    ImGui::End();
}
