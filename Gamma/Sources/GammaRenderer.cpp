#include "GammaRenderer.hpp"
#include "GLProgram.hpp"
#include "utils.hpp"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

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

                uniform vec3 cameraPos;

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

                    vec3 N = normalize(Normal);
                    vec3 V = normalize(cameraPos - WorldPos);
                    color *= abs(dot(N, V));

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
