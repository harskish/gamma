#include "Light.hpp"
#include "utils.hpp"
#include "Scene.hpp"
#include <glm/gtc/matrix_transform.hpp>


PointLight::PointLight(glm::vec3 pos, glm::vec3 e) {
    this->vector = glm::vec4(pos, 1.0f);
    this->emission = e;
    this->shadowMapDims = glm::uvec2(1024);
    initShadowMap(this->shadowMapDims);
}

void PointLight::initShadowMap(glm::uvec2 dims) {
    // Reallocate?
    if (dims != this->shadowMapDims) {
        this->shadowMapDims = dims;
        freeGLData();
    }

    if (dims.x > 0 && dims.y > 0) {
        // Create objects
        glGenFramebuffers(1, &shadowMapFBO);
        glGenTextures(1, &shadowMap);

        // Create depth texture
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowMap);
        for (int i = 0; i < 6; i++) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                shadowMapDims.x, shadowMapDims.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_BORDER);

        // Cubemap faces rendered by geometry shader trick, so one attachment suffices
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowMap, 0); // not 2D!
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glCheckError();
    }
}

// Uses geometry shader to generate 6 cube faces in one render pass
void PointLight::renderShadowMap(std::shared_ptr<Scene> scene) {
    std::string progId = "Render::shadowPoint";
    GLProgram* prog = GLProgram::get(progId);
    if (!prog) {
        prog = new GLProgram(readFile("Gamma/Shaders/shadowmap_point.vert"),
                             readFile("Gamma/Shaders/shadowmap_point.geom"),
                             readFile("Gamma/Shaders/shadowmap_point.frag"));
        GLProgram::set(progId, prog);
    }

    if (shadowMapDims.x == 0 || shadowMapDims.y == 0)
        return;

    glViewport(0, 0, shadowMapDims.x, shadowMapDims.y);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    // Enable frontface culling to combat 'Peter Panning'
    GLint cullingMode;
    glGetIntegerv(GL_CULL_FACE_MODE, &cullingMode);
    glCullFace(GL_FRONT);
    prog->use();

    // Geometry shader
    prog->setUniform("shadowMatrices[0]", getLightTransform(0));
    prog->setUniform("shadowMatrices[1]", getLightTransform(1));
    prog->setUniform("shadowMatrices[2]", getLightTransform(2));
    prog->setUniform("shadowMatrices[3]", getLightTransform(3));
    prog->setUniform("shadowMatrices[4]", getLightTransform(4));
    prog->setUniform("shadowMatrices[5]", getLightTransform(5));
    prog->setUniform("shadowMatrices[6]", getLightTransform(6));
    glCheckError();

    // Fragment shader
    prog->setUniform("lightPos", glm::vec3(this->vector));
    glCheckError();

    for (Model &m : scene->models()) {
        m.renderUnshaded(prog); // sets M in vertex shader
    }

    glCullFace(cullingMode);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();
}

glm::mat4 PointLight::getLightTransform(int face) {
    float aspect = (float)shadowMapDims.x / (float)shadowMapDims.y;
    float znear = 0.1f;
    float zfar = 25.0f;
    glm::mat4 P = glm::perspective(glm::radians(90.0f), aspect, znear, zfar);

    glm::vec3 lightPos = glm::vec3(this->vector);
    glm::mat4 V;
    switch (face)
    {
    case 0:
        V = glm::lookAt(lightPos, lightPos + glm::vec3(1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
        break;
    case 1:
        V = glm::lookAt(lightPos, lightPos + glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, -1.0, 0.0));
        break;
    case 2:
        V = glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 1.0, 0.0), glm::vec3(0.0, 0.0, 1.0));
        break;
    case 3:
        V = glm::lookAt(lightPos, lightPos + glm::vec3(0.0, -1.0, 0.0), glm::vec3(0.0, 0.0, -1.0));
        break;
    case 4:
        V = glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, 1.0), glm::vec3(0.0, -1.0, 0.0));
        break;
    case 5:
        V = glm::lookAt(lightPos, lightPos + glm::vec3(0.0, 0.0, -1.0), glm::vec3(0.0, -1.0, 0.0));
        break;
    }

    return P * V;
}


DirectionalLight::DirectionalLight(glm::vec3 dir, glm::vec3 e) {
    this->vector = glm::vec4(dir, 0.0f);
    this->emission = e;
    this->shadowMapDims = glm::uvec2(1024);
    initShadowMap(this->shadowMapDims);
}

void DirectionalLight::initShadowMap(glm::uvec2 dims) {
    // Reallocate?
    if (dims != this->shadowMapDims) {
        this->shadowMapDims = dims;
        freeGLData();
    }

    if (dims.x > 0 && dims.y > 0) {
        // Create objects
        glGenFramebuffers(1, &shadowMapFBO);
        glGenTextures(1, &shadowMap);

        // Create depth texture
        glBindTexture(GL_TEXTURE_2D, shadowMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
            shadowMapDims.x, shadowMapDims.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        const float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        // Bind depth texture, finalize FBO
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glCheckError();
    }
}

void DirectionalLight::renderShadowMap(std::shared_ptr<Scene> scene) {
    std::string progId = "Render::shadowDir";
    GLProgram* prog = GLProgram::get(progId);
    if (!prog) {
        prog = new GLProgram(readFile("Gamma/Shaders/shadowmap_dir.vert"),
                             readFile("Gamma/Shaders/shadowmap_dir.frag"));
        GLProgram::set(progId, prog);
    }
    
    if (shadowMapDims.x == 0 || shadowMapDims.y == 0)
        return;

    glViewport(0, 0, shadowMapDims.x, shadowMapDims.y);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    
    // Enable frontface culling to combat 'Peter Panning'
    GLint cullingMode;
    glGetIntegerv(GL_CULL_FACE_MODE, &cullingMode);
    glCullFace(GL_FRONT);

    prog->use();
    prog->setUniform("lightSpaceMatrix", getLightTransform());
    glCheckError();

    for (Model &m : scene->models()) {
        m.renderUnshaded(prog); // sets uniform named M in shader
    }

    glCullFace(cullingMode);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();
}

// Aimed at origin for now
// TODO: https://www.gamedev.net/forums/topic/537185-shadow-mapping-a-directional-light/?do=findComment&comment=4469364
glm::mat4 DirectionalLight::getLightTransform(int face) {
    (void)face;
    float scale = 1.5f;
    float near_plane = 0.1f, far_plane = 2 * scale;
    glm::mat4 P = glm::ortho(-scale, scale, -scale, scale, near_plane, far_plane);
    glm::vec3 target = glm::vec3(0.0f);
    glm::vec3 direction = glm::vec3(vector);
    glm::vec3 virtLightPos = target - direction;
    glm::mat4 V = glm::lookAt(virtLightPos, target, glm::vec3(0.0f, 1.0f, 0.0f));
    return P * V;
}
