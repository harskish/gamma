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
    std::string progId = "Render::shadeGGX";
    GLProgram* prog = GLProgram::get(progId);
    if (!prog) {
        std::cout << "Compiling GGX program" << std::endl;
        std::map<std::string, std::string> repl;
        repl["$MAX_LIGHTS"] = "#define MAX_LIGHTS " + std::to_string(MAX_LIGHTS);
        prog = new GLProgram(readFile("Gamma/Shaders/ggx.vert", repl),
                             readFile("Gamma/Shaders/ggx.frag", repl));
        GLProgram::set(progId, prog);
    }

    // Draw image
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
    std::vector<Light> &lights = scene->lights();
    prog->setUniform("nLights", (unsigned int)lights.size());
    for (size_t i = 0; i < lights.size(); i++) {
        Light &l = lights[i];
        prog->setUniform("lightVectors[" + std::to_string(i) + "]", l.vector);
        prog->setUniform("emissions[" + std::to_string(i) + "]", l.emission);
    }
    glCheckError();

    // Setup texture locations (these are static)
    prog->setUniform("albedoMap", 0);
    prog->setUniform("normalMap", 1);
    prog->setUniform("shininessMap", 2);
    prog->setUniform("metallicMap", 3);

    // M set by each model before drawing
    for (Model &m : scene->models()) {
        m.render(prog);
    }
}

GammaRenderer::GammaRenderer(GLFWwindow * w) {
    this->window = w;
    this->scene.reset(new Scene()); // default empty scene

    glDisable(GL_CULL_FACE);
    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_BACK);
    glEnable(GL_DEPTH_TEST);
}

void GammaRenderer::reshape() {
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    
    // Create new textures etc.
    std::cout << "New FB size: " << w << "x" << h << std::endl;
}
