#include "IBLMaps.hpp"
#include "GLProgram.hpp"
#include "xxhash.h"
#include "stb_image.h"
#include "utils.hpp"
#include <glm/gtc/matrix_transform.hpp>

IBLMaps::IBLMaps(std::string mapName, float brightness) {
    size_t hash = fileHash(mapName);
    this->brightnessScale = brightness;

    // Try to load pre-processed environment
    std::ifstream f("Gamma/Assets/IBL/cached/" + std::to_string(hash) + ".ktx");
    if (f.good()) {
        loadCached(f);
    }
    else {
        process(mapName);
    }
}

IBLMaps::~IBLMaps() {
    glDeleteTextures(1, &brdfMap);
    glDeleteTextures(1, &radianceMap);
    glDeleteTextures(1, &irradianceMap);
    glDeleteTextures(1, &backgroundMap);
}

void IBLMaps::process(std::string path) {    
    // Save current viewport to restore later
    GLint viweport[4];
    glGetIntegerv(GL_VIEWPORT, viweport);
    
    // Load HDR environment texture
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float *data = stbi_loadf(path.c_str(), &width, &height, &nrComponents, 0);
    stbi_set_flip_vertically_on_load(false);
    if (!data) {
        throw std::runtime_error("Failed to load " + path);
    }

    // Scale brightness
    if (brightnessScale != 1.0f)
        std::for_each(data, data + (width * height * nrComponents), [&](float &v) { v *= this->brightnessScale; });

    GLuint hdrTexture = 0;
    glGenTextures(1, &hdrTexture);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    stbi_image_free(data);

    // Interpolation over cubemap edges to fix cubemap edge artifacts
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    // Create FBO, RBO for processing
    glGenFramebuffers(1, &FBO);
    glGenRenderbuffers(1, &RBO);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, RBO);
    glCheckError();

    // Convert equirectangular map to cubemap for speed and interpolation
    backgroundMap = equirecToCubemap(hdrTexture);
    glDeleteTextures(1, &hdrTexture);

    // Convolve => irradiance texture (diffuse)
    createIrradianceMap();

    // Convolve => radiance texture (specular)
    createRadianceMap();

    // Create BRDF lookup texture (specular)
    createBrdfLUT();

    // Reset state
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &FBO);
    glDeleteRenderbuffers(1, &RBO);
    glViewport(viweport[0], viweport[1], viweport[2], viweport[3]);
}

void IBLMaps::loadCached(std::ifstream &stream) {
    throw std::runtime_error("IBL cache loading not implemented");
}

// Get view matrix for looking at cubemap face i
glm::mat4 IBLMaps::lookAtFace(unsigned int i) {
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

GLuint IBLMaps::equirecToCubemap(GLuint srcTex) {
    GLuint CUBE;
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
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glCheckError();

    // Fill view with cubemap face
    glm::mat4 P = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

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
        progUnwrap->setUniform("V", lookAtFace(i));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, CUBE, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawUnitCube();
    }

    // Now generate mipmaps (for mipmap sampling in radiance concolution step)
    glBindTexture(GL_TEXTURE_CUBE_MAP, CUBE);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    glCheckError();

    return CUBE;
}

void IBLMaps::createIrradianceMap() {
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    for (unsigned int i = 0; i < 6; i++) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 32, 32, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Irradiance => low resolution requirement
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 32, 32);

    // Fill view
    glm::mat4 P = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

    GLProgram *irrProg = getProgram("IBL::Irradiance", "eq_cube_unwrap.vert", "ibl_calc_irradiance.frag");
    irrProg->use();
    irrProg->setUniform("P", P);
    irrProg->setUniform("environmentMap", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, backgroundMap);
    glCheckError();

    glViewport(0, 0, 32, 32);
    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    for (unsigned int i = 0; i < 6; i++) {
        irrProg->setUniform("V", lookAtFace(i));
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawUnitCube();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();
}

void IBLMaps::createRadianceMap() {
    glGenTextures(1, &radianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, radianceMap);
    for (unsigned int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP); // roughnesses stored in mip-levels

    // Fill view
    glm::mat4 P = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);

    GLProgram *radProg = getProgram("IBL::Radiance", "eq_cube_unwrap.vert", "ibl_calc_radiance.frag");
    radProg->use();
    radProg->setUniform("P", P);
    radProg->setUniform("environmentMap", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, backgroundMap);
    glCheckError();

    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; mip++) {
        unsigned int mipWidth = 128 * std::pow(0.5, mip);
        unsigned int mipHeight = 128 * std::pow(0.5, mip);
        glBindRenderbuffer(GL_RENDERBUFFER, RBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        radProg->setUniform("roughness", roughness);
        for (unsigned int i = 0; i < 6; i++) {
            radProg->setUniform("V", lookAtFace(i));
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, radianceMap, mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            drawUnitCube();
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glCheckError();
}

void IBLMaps::createBrdfLUT() {
    glGenTextures(1, &brdfMap);
    glBindTexture(GL_TEXTURE_2D, brdfMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, FBO);
    glBindRenderbuffer(GL_RENDERBUFFER, RBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfMap, 0);

    glViewport(0, 0, 512, 512);
    GLProgram *brdfProg = getProgram("IBL::BrdfLUT", "draw_tex_2d.vert", "ibl_calc_brdf_lut.frag");
    brdfProg->use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawFullscreenQuad();
    glCheckError();
}
