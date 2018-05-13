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

    glCheckError();
}

GammaRenderer::GammaRenderer(GLFWwindow * w) {
    this->window = w;
    this->scene.reset(new Scene()); // default empty scene
    this->bloomPass.reset(new BloomPass());

    glDisable(GL_CULL_FACE);
    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);

    genQueryBuffers();
    reshape(); // sets up FBO and Bloom filter
}

void GammaRenderer::reshape() {
    glfwGetFramebufferSize(window, &windowWidth, &windowHeight);
    fbWidth = static_cast<int>(renderScale * windowWidth);
    fbHeight = static_cast<int>(renderScale * windowHeight);
    
    // Create new textures
    setupFBO();
    this->bloomPass->resize(fbWidth, fbHeight);
}

void GammaRenderer::shadowPass() {
    glBeginQuery(GL_TIME_ELAPSED, queryID[queryBackBuffer][0]);
    
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

    glEndQuery(GL_TIME_ELAPSED);
    glCheckError();
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
    // Viewport uses FB size, not window size
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

    // Setup IBL maps
    auto maps = scene->getIBLMaps();
    prog->setUniform("irradianceMap", 4);
    prog->setUniform("radianceMap", 5);
    prog->setUniform("brdfLUT", 6);
    glActiveTexture(GL_TEXTURE4);
    glBindTexture(GL_TEXTURE_CUBE_MAP, maps->getIrradianceMap());
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_CUBE_MAP, maps->getRadianceMap());
    glActiveTexture(GL_TEXTURE6);
    glBindTexture(GL_TEXTURE_2D, maps->getBrdfLUT());

    // Setup other parameters
    prog->setUniform("useVSM", Light::useVSM);
    prog->setUniform("svmBleedFix", Light::svmBleedFix);

    // M set by each model before drawing
    glBeginQuery(GL_TIME_ELAPSED, queryID[queryBackBuffer][1]);
    {
        for (Model &m : scene->models()) {
            m.render(prog);
        }

        drawSkybox();
    }
    glEndQuery(GL_TIME_ELAPSED);
    glCheckError();

    // Reset viewport to window size for later passes
    glViewport(0, 0, windowWidth, windowHeight);
}

void GammaRenderer::postProcessPass() {
    GLProgram* progFXAA = getProgram("PP::FXAA", "draw_tex_2d.vert", "filter_fxaa.frag");
    GLProgram* progTonemap = getProgram("PP:Tonemap", "draw_tex_2d.vert", "filter_tonemap.frag");
    GLProgram* progNull = getProgram("PP::null", "draw_tex_2d.vert", "filter_null.frag");

    std::vector<GLProgram*> passes;

    progTonemap->use();
    progTonemap->setUniform("useUC2", tonemapOp == 0);
    progTonemap->setUniform("exposure", tonemapExposure);
    passes.push_back(progTonemap);
    
    // FXAA after tone mapping!
    if (useFXAA) {
        progFXAA->use();
        progFXAA->setUniform("invTexSize", glm::vec2(1.0f / fbWidth, 1.0f / fbHeight));
        progFXAA->setUniform("fxaaSpanMax", 8.0f);
        progFXAA->setUniform("fxaaReduceMul", 1.0f / 8.0f);
        progFXAA->setUniform("fxaaReduceMin", 1.0f / 128.0f);
        passes.push_back(progFXAA);
    }

    glViewport(0, 0, fbWidth, fbHeight);
    glBeginQuery(GL_TIME_ELAPSED, queryID[queryBackBuffer][2]);

    if (useBloom) {
        bloomPass->apply(colorTex[colorDst], fbo);
    }
    
    size_t nPasses = passes.size();
    for (int i = 0; i < nPasses; i++) {
        if (i == nPasses - 1) {
            glViewport(0, 0, windowWidth, windowHeight);
            applyFilter(passes[i], colorTex[colorDst], 0, 0); // to screen
        }
        else {
            applyFilter(passes[i], colorTex[colorDst], colorTex[1 - colorDst], fbo);
            colorDst = 1 - colorDst; // ping pong
        }
    }

    colorDst = 0;
    
    glEndQuery(GL_TIME_ELAPSED);
    glCheckError();
}

void GammaRenderer::drawSkybox() {
    auto maps = scene->getIBLMaps();
    GLuint skybox = maps->getBackgroundMap();
    if (skybox > 0) {
        GLProgram *bgProg = getProgram("Render::skybox", "draw_skybox_hdr.vert", "draw_skybox_hdr.frag");
        bgProg->use();
        bgProg->setUniform("V", camera->getV());
        bgProg->setUniform("P", camera->getP());
        bgProg->setUniform("envMap", 0);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, skybox);
        glDepthFunc(GL_LEQUAL);
        drawUnitCube();
        
        glDepthFunc(GL_LESS);
    }
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
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F,
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
    checkFBStatus();

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

    const int LEN = 90;
    static float shadowTimes[LEN] = { 0 };
    static float shadingTimes[LEN] = { 0 };
    static float postprocTimes[LEN] = { 0 };
    static int offs = 0;
    static bool firstFrame = true;

    GLuint64 shadowsNs = 0;
    GLuint64 shadingNs = 0;
    GLuint64 postprocessNs = 0;
    if (!firstFrame) {
        glGetQueryObjectui64v(queryID[queryFrontBuffer][0], GL_QUERY_RESULT, &shadowsNs);
        glGetQueryObjectui64v(queryID[queryFrontBuffer][1], GL_QUERY_RESULT, &shadingNs);
        glGetQueryObjectui64v(queryID[queryFrontBuffer][2], GL_QUERY_RESULT, &postprocessNs);
    }
    glCheckError();
    swapQueryBuffers();

    // Calculate average (last N samples)
    auto smooth = [LEN](float *arr) {
        const int N = 20;
        float sum = 0.0f;
        for (int i = 0; i < N; i++) {
            int idx = offs - 1 - i;
            if (idx < 0) idx += LEN;
            sum += arr[idx];
        }
        return sum / N;
    };

    shadowTimes[offs] = (float)(shadowsNs / 1e6);
    shadingTimes[offs] = (float)(shadingNs / 1e6);
    postprocTimes[offs] = (float)(postprocessNs / 1e6);
    offs = (offs + 1) % LEN;
    
    char labelShadow[20];
    sprintf(labelShadow, "Avg: %.2fms", smooth(shadowTimes));
    ImGui::PlotLines(labelShadow, shadowTimes, LEN, offs, "Shadows (ms)", 0.0f, 10.0f, ImVec2(0, 80));
    
    char labelShading[20];
    sprintf(labelShading, "Avg: %.2fms", smooth(shadingTimes));
    ImGui::PlotLines(labelShading, shadingTimes, LEN, offs, "Shading (ms)", 0.0f, 10.0f, ImVec2(0, 80));

    char labelPostproc[20];
    sprintf(labelPostproc, "Avg: %.2fms", smooth(postprocTimes));
    ImGui::PlotLines(labelPostproc, postprocTimes, LEN, offs, "Postprocessing (ms)", 0.0f, 10.0f, ImVec2(0, 80));

    firstFrame = false;
    ImGui::End();
}

// General renderer settings window
void GammaRenderer::drawSettings(bool * show) {
    if (!ImGui::Begin("Renderer settings", show)) {
        ImGui::End();
        return;
    }

    if (ImGui::CollapsingHeader("Performance")) {
        static int scale = (int)(renderScale * 100.0f);
        if (ImGui::SliderInt("Render scale", &scale, 10, 150, "%.0f%%")) {
            renderScale = (float)(scale / 100.0f);
            reshape();
        }

        std::string fbdims = "Framebuffer size: " + std::to_string(fbWidth) + "x" + std::to_string(fbHeight);
        ImGui::Text(fbdims.c_str());

        ImGui::Checkbox("Use FXAA", &useFXAA);
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


    if (ImGui::CollapsingHeader("Tonemapping")) {
        ImGui::SliderFloat("Exposure", &tonemapExposure, 0.0f, 16.0f, "%.3f", 3.0f); // non-linear slider
        ImGui::RadioButton("Uncharted 2", &tonemapOp, 0); ImGui::SameLine();
        ImGui::RadioButton("Reinhard", &tonemapOp, 1);
    }

    if (ImGui::CollapsingHeader("Bloom")) {
        ImGui::Checkbox("Enabled", &useBloom);
        ImGui::SliderFloat("Threshold", &bloomPass->threshold, 0.0f, 100.0f, "%.3f", 2.0f);
        ImGui::SliderFloat("Radius", &bloomPass->radius, 0.35f, 4.0f, "%.3f", 1.0f);
        ImGui::SliderFloat("Strength", &bloomPass->strength, 0.0f, 3.0f, "%.3f", 1.0f);
        ImGui::Checkbox("Karis avg (AA)", &bloomPass->useAA);
    }

    ImGui::End();
}
