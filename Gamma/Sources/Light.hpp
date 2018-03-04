#pragma once
#include <memory>
#include <glm/glm.hpp>
#include <glad/glad.h>
#include "utils.hpp"

class Scene;
class Light {
public:

    ~Light() {
        freeGLData();
    }

    glm::vec4 getVector() { return vector; }
    glm::vec3 getEmission() { return emission; }
    GLuint getTexHandle() { return shadowMap; }

    virtual void initShadowMap(glm::uvec2 dims) = 0;
    virtual void renderShadowMap(std::shared_ptr<Scene> scene) = 0;
    virtual glm::mat4 getLightTransform(int face = 0) = 0; // TODO: cache result!

    glm::vec4 vector; // w=0 => directional, w!=0 => positional
    glm::vec3 emission;

protected:
    void freeGLData() {
        glDeleteTextures(1, &shadowMap);
        glDeleteFramebuffers(1, &shadowMapFBO);
        glCheckError();
    }

    glm::uvec2 shadowMapDims;
    GLuint shadowMap = 0;
    GLuint shadowMapFBO = 0;
};

class PointLight : public Light {
public:
    PointLight(void) : Light() {}
    PointLight(glm::vec3 pos, glm::vec3 e);
    void initShadowMap(glm::uvec2 dims) override;
    void renderShadowMap(std::shared_ptr<Scene> scene) override;
    glm::mat4 getLightTransform(int face = 0) override;
};

class DirectionalLight : public Light {
public:
    DirectionalLight(void) : Light() {}
    DirectionalLight(glm::vec3 dir, glm::vec3 e);
    void initShadowMap(glm::uvec2 dims) override;
    void renderShadowMap(std::shared_ptr<Scene> scene) override;
    glm::mat4 getLightTransform(int face = 0) override;
};