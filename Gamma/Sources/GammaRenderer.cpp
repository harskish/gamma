#include "GammaRenderer.hpp"
#include "GLProgram.hpp"
#include "utils.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void GammaRenderer::render() {
    const char* progId = "Test::drawSquare";
    GLProgram* prog = GLProgram::get(progId);
    if (!prog) {
        prog = new GLProgram(
            "#version 330\n"
            GL_SHADER_SOURCE(
                uniform mat4 MVP;
                layout(location = 0) in vec3 posAttrib;
                void main() {
                    gl_Position = MVP * vec4(posAttrib, 1.0);
                }
            ),
            "#version 330\n"
            GL_SHADER_SOURCE(
                out vec4 fragColor;
                void main() {
                    fragColor = vec4(1.0, 0.0, 0.0, 1.0);
                }
            )
        );

        // Update static shader storage
        GLProgram::set(progId, prog);
    }

    // Draw image
    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Viewport
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);

    // Build MVP matrix
    glm::vec3 T(0.0f, 0.0f, -3.0f);
    glm::mat4 P = glm::perspective(glm::radians(65.0f), (float)w / (float)h, 0.1f, 100.f);
    glm::mat4 VT = glm::translate(glm::mat4(1.0f), T);
    glm::mat4 V = glm::rotate(VT, (float)glfwGetTime(), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 M = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));
    glm::mat4 MVP = P * V * M;
    
    prog->use();
    prog->setUniform(prog->getUniformLoc("MVP"), MVP);

    for (Model &m : scene->models()) {
        m.render();
        glCheckError();
    }
}

GammaRenderer::GammaRenderer(GLFWwindow * w) {
    this->window = w;
    this->scene.reset(new Scene());
}

void GammaRenderer::reshape() {
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    
    // Create new textures etc.
    std::cout << "New FB size: " << w << "x" << h << std::endl;
}
