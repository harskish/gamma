#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>

class IBLMaps {
public:
    IBLMaps(void) = default;
    IBLMaps(std::string mapName, float brightness = 1.0f);
    ~IBLMaps();

    GLuint getBrdfLUT() { return brdfMap; }
    GLuint getRadianceMap() { return radianceMap; }
    GLuint getIrradianceMap() { return irradianceMap; }
    GLuint getBackgroundMap() { return backgroundMap; }

private:
    void process(std::string path);
    void loadCached(std::ifstream &stream);

    glm::mat4 lookAtFace(unsigned int i);

    // Convert equirectangular to cubemap, return texture handle
    GLuint equirecToCubemap(GLuint srcTex);
    void createIrradianceMap();
    void createRadianceMap();
    void createBrdfLUT();

    float brightnessScale = 1.0f;

    GLuint brdfMap = 0;
    GLuint radianceMap = 0;
    GLuint irradianceMap = 0;
    GLuint backgroundMap = 0;

    GLuint FBO, RBO;
};
