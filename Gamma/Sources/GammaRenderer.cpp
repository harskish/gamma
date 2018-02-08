#include "GammaRenderer.hpp"
#include "GLProgram.hpp"
#include "utils.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

void GammaRenderer::render(void) {
    const GLfloat posAttrib[] = {
        -1.0f, -1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 0.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f,  1.0f, 0.0f, 1.0f,
    };

    const char* progId = "Test::drawSquare";
    GLProgram* prog = GLProgram::get(progId);
    if (!prog) {
        prog = new GLProgram(
            "#version 330\n"
            GL_SHADER_SOURCE(
                uniform mat4 MVP;
                layout(location = 0) in vec4 posAttrib;
                void main() {
                    gl_Position = MVP * posAttrib;
                }
                ),
            "#version 330\n"
            GL_SHADER_SOURCE(
                out vec4 fragColor;
                void main() {
                    fragColor = vec4(1.0, 0.0, 0.0, 0.5);
                }
            )
        );

        // Update static shader storage
        GLProgram::set(progId, prog);

        // Setup VAO
        GLuint vbo[1], vao[1];
        glGenBuffers(1, vbo); // VBO: stores single attribute (pos/color/normal etc.)
        glGenVertexArrays(1, vao); // VAO: bundles VBO:s together
        glBindVertexArray(vao[0]);
        glCheckError();

        // Positions
        glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
        glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), posAttrib, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0); // coordinate data in attribute index 0, four floats per vertex
        glEnableVertexAttribArray(0); // enable index 0 within VAO
        glCheckError();

        prog->addVAOs(vao, 1);
    }

    // Draw image
    glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Viewport
    int w, h;
    glfwGetFramebufferSize(window, &w, &h);
    glViewport(0, 0, w, h);

    // Build MVP matrix
    glm::vec3 Translate(0.0f, 0.0f, -0.2f);
    glm::mat4 Projection = glm::perspective(60.0f, (float)w / (float)h, 0.1f, 100.f);
    glm::mat4 ViewTranslate = glm::translate(glm::mat4(1.0f), Translate);
    glm::mat4 View = glm::rotate(ViewTranslate, (float)glfwGetTime(), glm::vec3(0.0f, 0.0f, 1.0f));
    glm::mat4 Model = glm::scale(glm::mat4(1.0f), glm::vec3(0.5f));
    glm::mat4 MVP = Projection * View * Model;
    
    prog->use();
    prog->setUniform(prog->getUniformLoc("MVP"), MVP);
    prog->bindVAO(0);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glCheckError();
}
