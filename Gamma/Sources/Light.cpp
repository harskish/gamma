#include "Light.hpp"
#include "utils.hpp"
#include "Scene.hpp"
#include <glm/gtc/matrix_transform.hpp>

bool Light::useVSM = true;
int Light::defaultRes = 1024;
float Light::svmBleedFix = 0.2f;
float Light::svmBlur = 1.0f;

PointLight::PointLight(glm::vec3 pos, glm::vec3 e) {
    this->vector = glm::vec4(pos, 1.0f);
    this->emission = e;
    initShadowMap();
}

void PointLight::initShadowMap(glm::uvec2 dims) {
    // Remove old data
    this->shadowMapDims = dims;
    freeGLData();

    if (dims.x > 0 && dims.y > 0) {
        // Create objects
        glGenFramebuffers(1, &shadowMapFBO);
        glGenTextures(1, &shadowMap);
        glGenTextures(1, &momentMap);
        glGenTextures(1, &momentMapTmp);

        // Depth attachment always used
        glBindTexture(GL_TEXTURE_CUBE_MAP, shadowMap);
        for (int i = 0; i < 6; i++) {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT,
                shadowMapDims.x, shadowMapDims.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        // Cubemap faces rendered by geometry shader trick, so one attachment suffices
        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowMap, 0); // not 2D!
        glReadBuffer(GL_NONE);

        if (useVSM) {
            GLuint textures[2] = { momentMapTmp, momentMap };
            for (int tex = 0; tex < 2; tex++) {
                glBindTexture(GL_TEXTURE_CUBE_MAP, textures[tex]);
                for (int i = 0; i < 6; i++) {
                    glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RG32F,
                        shadowMapDims.x, shadowMapDims.y, 0, GL_RGBA, GL_FLOAT, NULL);
                }

                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
            }
            
            glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, momentMap, 0); // not 2D!
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glCheckError();
    }
}

// Uses geometry shader to generate 6 cube faces in one render pass
void PointLight::renderShadowMap(std::shared_ptr<Scene> scene) {
    GLProgram* prog = getProgram("Render::shadowPoint", "shadowmap_point.vert",
                                 "shadowmap_point.geom", "shadowmap_point.frag");

    if (shadowMapDims.x == 0 || shadowMapDims.y == 0)
        return;

    glViewport(0, 0, shadowMapDims.x, shadowMapDims.y);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);

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
    glCheckError();

    // Fragment shader
    prog->setUniform("lightPos", glm::vec3(this->vector));
    prog->setUniform("farPlane", 25.0f);
    prog->setUniform("useVSM", useVSM);
    glCheckError();

    for (Model &m : scene->models()) {
        m.renderUnshaded(prog); // sets M in vertex shader
    }

    glCullFace(cullingMode);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();
}

// Get view matrix for looking at cubemap face i
glm::mat4 lookAtFace(unsigned int i) {
    const glm::mat4 V[] = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    return V[i];
}

void PointLight::processShadowMap() {
    if (!useVSM) return;

    // Setup program
    GLProgram* prog = getProgram("SVM::CubeBlur7x1", "shadowmap_point.vert", "cube_blur_gauss_7x1.geom", "cube_blur_gauss_7x1.frag");
    prog->use();
    prog->setUniform("M", glm::mat4(1.0f)); // identity (unit cube at origin)

    float aspect = (float)shadowMapDims.x / (float)shadowMapDims.y;
    float znear = 0.1f;
    float zfar = 25.0f;
    glm::mat4 P = glm::perspective(glm::radians(90.0f), aspect, znear, zfar);

    prog->setUniform("shadowMatrices[0]", P * lookAtFace(0));
    prog->setUniform("shadowMatrices[1]", P * lookAtFace(1));
    prog->setUniform("shadowMatrices[2]", P * lookAtFace(2));
    prog->setUniform("shadowMatrices[3]", P * lookAtFace(3));
    prog->setUniform("shadowMatrices[4]", P * lookAtFace(4));
    prog->setUniform("shadowMatrices[5]", P * lookAtFace(5));

    prog->setUniform("sourceTexture", 0);
    glActiveTexture(GL_TEXTURE0);
    glCheckError();

    //glViewport(0, 0, shadowMapDims.x, shadowMapDims.y);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // Horizontal
    glBindTexture(GL_TEXTURE_CUBE_MAP, momentMap); // src
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, momentMapTmp, 0); // dst
    prog->setUniform("blurScale", glm::vec3(svmBlur / shadowMapDims.x, 0.0f, 0.0f));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawUnitCube();
    glCheckError();

    // Vertical
    glBindTexture(GL_TEXTURE_CUBE_MAP, momentMapTmp); // src
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, momentMap, 0); // dst
    prog->setUniform("blurScale", glm::vec3(0.0f, svmBlur / shadowMapDims.y, 0.0f));
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawUnitCube();
    glCheckError();

    // Restore state
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
    this->vector = glm::vec4(glm::normalize(dir), 0.0f);
    this->emission = e;
    initShadowMap();
}

void DirectionalLight::initShadowMap(glm::uvec2 dims) {
    // Remove old data
    this->shadowMapDims = dims;
    freeGLData();

    if (dims.x > 0 && dims.y > 0) {
        // Create objects
        glGenFramebuffers(1, &shadowMapFBO);
        glGenTextures(1, &shadowMap);
        glGenTextures(1, &momentMap);
        glGenTextures(1, &momentMapTmp);

        // Depth texture always used
        glBindTexture(GL_TEXTURE_2D, shadowMap);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT,
            shadowMapDims.x, shadowMapDims.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        const float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowMap, 0);
        glReadBuffer(GL_NONE);

        if (useVSM) {
            GLuint textures[2] = { momentMapTmp, momentMap };
            for (int i = 0; i < 2; i++) {
                glBindTexture(GL_TEXTURE_2D, textures[i]);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F,
                    shadowMapDims.x, shadowMapDims.y, 0, GL_RGBA, GL_FLOAT, NULL);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
                const float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };
                glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
            }

            // Color attachment for writing VSM moments
            glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, momentMap, 0);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glCheckError();
    }
}

void DirectionalLight::renderShadowMap(std::shared_ptr<Scene> scene) {
    GLProgram* prog = getProgram("Render::shadowDir", "shadowmap_dir.vert", "shadowmap_dir.frag");
    
    if (shadowMapDims.x == 0 || shadowMapDims.y == 0)
        return;

    glViewport(0, 0, shadowMapDims.x, shadowMapDims.y);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    
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

void DirectionalLight::processShadowMap() {
    if (!useVSM) return;

    GLProgram* prog = getProgram("SVM::Blur7x1", "draw_tex_2d.vert", "blur_gauss_7x1.frag");
    prog->use();

    // Horizontal
    prog->setUniform("blurScale", glm::vec2(svmBlur / shadowMapDims.x, 0.0f));
    applyFilter(prog, momentMap, momentMapTmp, shadowMapFBO);
    glCheckError();

    // Vertical
    prog->setUniform("blurScale", glm::vec2(0.0f, svmBlur / shadowMapDims.y));
    applyFilter(prog, momentMapTmp, momentMap, shadowMapFBO);
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
