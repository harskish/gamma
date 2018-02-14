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
                layout(location = 0) in vec3 posAttrib;
                layout(location = 1) in vec3 normAttrib;
                layout(location = 2) in vec2 texAttrib;

                out vec2 TexCoords;
                out vec3 WorldPos;
                out vec3 Normal;
                
                uniform mat4 P;
                uniform mat4 V;
                uniform mat4 M;

                void main() {
                    TexCoords = texAttrib;
                    WorldPos = vec3(M * vec4(posAttrib, 1.0));
                    Normal = mat3(M) * normAttrib;

                    //gl_Position = MVP * vec4(posAttrib, 1.0);
                    gl_Position = P * V * vec4(WorldPos, 1.0);
                }
                ),
            "#version 330\n"
            GL_SHADER_SOURCE(
                const uint DIFFUSE_MASK = (1U << 0);
                const uint NORMAL_MASK = (1U << 1);
                const uint SHININESS_MASK = (1U << 2);
                const uint METALLIC_MASK = (1U << 3);
                const uint BUMP_MASK = (1U << 4);
                const uint DISPLACEMENT_MASK = (1U << 5);
                const uint EMISSION_MASK = (1U << 6);

                out vec4 FragColor;
                in vec2 TexCoords;
                in vec3 WorldPos;
                in vec3 Normal;

                // Mesh's global material
                uniform vec3 Kd;
                uniform float metallic;
                uniform float shininess;
                uniform uint texMask;
                
                // Override parameters per fragment
                uniform sampler2D albedoMap;
                uniform sampler2D normalMap;
                uniform sampler2D shininessMap;
                uniform sampler2D metallicMap;

                void main() {
                    vec3 color = Kd;
                    
                    if ((texMask & DIFFUSE_MASK) != 0U) {
                        color = texture(albedoMap, TexCoords).rgb;
                    }

                    FragColor = vec4(color, 1.0);
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
    prog->setUniform("M", M);
    prog->setUniform("V", V);
    prog->setUniform("P", P);
    glCheckError();

    for (Model &m : scene->models()) {
        m.render(prog);
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
