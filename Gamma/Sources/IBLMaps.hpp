#pragma once
#include <glad/glad.h>
#include <string>

class IBLMaps {
public:
    IBLMaps(void) = default;
    IBLMaps(std::string mapName);
    ~IBLMaps();

    GLuint getBrdfLUT() { return brdfMap; }
    GLuint getRadianceMap() { return irradianceMap; }
    GLuint getIrradianceMap() { return radianceMap; }
    GLuint getBackgroundMap() { return backgroundMap; }

private:
    void process(std::string path);
    void loadCached(std::ifstream &stream);

    // Convert equirectangular to cubemap, return texture handle
    GLuint equirecToCubemap(GLuint srcTex);

    GLuint brdfMap = 0;
    GLuint radianceMap = 0;
    GLuint irradianceMap = 0;
    GLuint backgroundMap = 0;
};