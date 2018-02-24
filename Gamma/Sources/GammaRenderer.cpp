#include "GammaRenderer.hpp"
#include "GLProgram.hpp"
#include "utils.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

void GammaRenderer::render() {
    std::string progId = "Render::shadeGGX";
    GLProgram* prog = GLProgram::get(progId);
    if (!prog) {
        std::cout << "Compiling GGX program" << std::endl;
        prog = new GLProgram(readFile("Gamma/Shaders/ggx.vert"),
                             readFile("Gamma/Shaders/ggx.frag"));
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

    // M set by each model before drawing
    for (Model &m : scene->models()) {
        m.render(prog);
    }
}

GammaRenderer::GammaRenderer(GLFWwindow * w) {
    this->window = w;
    this->scene.reset(new Scene());

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
