#include "IBLMaps.hpp"
#include "GLProgram.hpp"
#include "xxhash.h"
#include "stb_image.h"
#include "utils.hpp"
#include <glm/gtc/matrix_transform.hpp>

IBLMaps::IBLMaps(std::string mapName) {
    std::string path = "Gamma/Assets/IBL/";
    size_t hash = fileHash(path + mapName);

    // Try to load pre-processed environment
    std::ifstream f("Gamma/Assets/IBL/cached/" + std::to_string(hash) + ".ktx");
    if (f.good()) {
        loadCached(f);
    }
    else {
        process(path + mapName);
    }
}

IBLMaps::~IBLMaps() {
    glDeleteTextures(1, &brdfMap);
    glDeleteTextures(1, &radianceMap);
    glDeleteTextures(1, &irradianceMap);
    glDeleteTextures(1, &backgroundMap);
}

void IBLMaps::process(std::string path) {
    GLuint hdrTexture = 0;
    
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float *data = stbi_loadf(path.c_str(), &width, &height, &nrComponents, 0);
    stbi_set_flip_vertically_on_load(false);
    if (!data){
        throw std::runtime_error("Failed to load " + path);
    }

    glGenTextures(1, &hdrTexture);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);

    // Convert equirectangular map to cubemap for speed and interpolation
    backgroundMap = equirecToCubemap(hdrTexture);
    glDeleteTextures(1, &hdrTexture);

    // Convolve => irradiance texture



    // Convolve => radiance texture


    // Create BRDF lookup texture

}

void IBLMaps::loadCached(std::ifstream &stream) {
    throw std::runtime_error("IBL cache loading not implemented");
}

GLuint IBLMaps::equirecToCubemap(GLuint srcTex) {
    GLuint FBO, RBO, CUBE;
    glGenFramebuffers(1, &FBO);
    glGenRenderbuffers(1, &RBO);
    glGenTextures(1, &CUBE);

    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, RBO);
    glCheckError();

    glBindTexture(GL_TEXTURE_CUBE_MAP, CUBE);
    for (unsigned int i = 0; i < 6; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 512, 512, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glCheckError();

    // Look at each face of cubemap for rendering
    glm::mat4 P = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 V[] = {
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f,  0.0f,  0.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  1.0f,  0.0f), glm::vec3(0.0f,  0.0f,  1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f,  0.0f), glm::vec3(0.0f,  0.0f, -1.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f,  1.0f), glm::vec3(0.0f, -1.0f,  0.0f)),
        glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f,  0.0f, -1.0f), glm::vec3(0.0f, -1.0f,  0.0f))
    };

    GLProgram* progUnwrap = getProgram("IBL::UnwrapEQ", "eq_cube_unwrap.vert", "eq_cube_unwrap.frag");
    progUnwrap->use();
    progUnwrap->setUniform("P", P);
    progUnwrap->setUniform("equirecTex", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, srcTex);

    // Render
    glViewport(0, 0, 512, 512);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    for (unsigned int i = 0; i < 6; i++) {
        progUnwrap->setUniform("V", V[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, CUBE, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawUnitCube();
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteRenderbuffers(1, &RBO);
    glDeleteFramebuffers(1, &FBO);
    glCheckError();

    return CUBE;
}
