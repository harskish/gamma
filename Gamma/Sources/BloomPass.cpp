#include "BloomPass.hpp"
#include "GLProgram.hpp"
#include "imgui.h"
#include "utils.hpp"
#include <cmath>

BloomPass::BloomPass(int width, int height) {
    resize(width, height);
}

void BloomPass::resize(int w, int h) {
    this->width = w;
    this->height = h;
    
    // Remove previous textures
    freeGLData();

    // Limit smallest MIP dim to 16px
    const int defaultDepth = 6;
    int dim = std::min(width, height);
    int depthMax = std::max(1, (int)log2(dim) - (int)log2(16));
    this->mipChainDepth = std::min(defaultDepth, depthMax);

    glGenFramebuffers(mipChainDepth, FBOS);
    glGenTextures(mipChainDepth, textures);

    // Resolution halved between MIP levels
    for (int mip = 0; mip < mipChainDepth; mip++) {
        unsigned int w = width * std::pow(0.5, mip);
        unsigned int h = height * std::pow(0.5, mip);

        glBindTexture(GL_TEXTURE_2D, textures[mip]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, NULL); // R11G11B10 gives minimal gains
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, FBOS[mip]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textures[mip], 0);
        checkFBStatus();
        glCheckError();
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glCheckError();
}

void BloomPass::apply(GLuint inputTex, GLuint dstFBO) {
    GLProgram* bloomThreshold = getProgram("PP::BloomTH", "draw_tex_2d.vert", "bloom_th.frag");
    GLProgram* bloomDownsample = getProgram("PP::BloomDS", "draw_tex_2d.vert", "bloom_ds.frag");
    GLProgram* bloomUpsample = getProgram("PP:BloomUS", "draw_tex_2d.vert", "bloom_us.frag");
    GLProgram* bloomAdd = getProgram("Tex::Sum", "draw_tex_2d.vert", "texture_sum.frag");

    if (mipChainDepth == 0) {
        std::cout << "Bloom filter not initialized!" << std::endl;
    }

    bloomThreshold->use();
    bloomThreshold->setUniform("sourceTexture", 0);
    bloomThreshold->setUniform("threshold", threshold);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTex);

    glBindFramebuffer(GL_FRAMEBUFFER, FBOS[0]);
    glViewport(0, 0, this->width, this->height);
    drawFullscreenQuad();

    /* Downsampling pass */
    bloomDownsample->use();
    bloomDownsample->setUniform("sourceTexture", 0);
    for (int i = 1; i < mipChainDepth; i++) {
        unsigned int wSrc = width * std::pow(0.5, i - 1);
        unsigned int hSrc = height * std::pow(0.5, i - 1);
        unsigned int wDst = width * std::pow(0.5, i);
        unsigned int hDst = height * std::pow(0.5, i);
        bloomDownsample->setUniform("useKarisAvg", useAA && (i == 1)); // AA applied on first downsample
        bloomDownsample->setUniform("invRes", glm::vec2(1.0 / wSrc, 1.0 / hSrc));
        glBindTexture(GL_TEXTURE_2D, textures[i - 1]); // previous level as input
        glBindFramebuffer(GL_FRAMEBUFFER, FBOS[i]);
        glViewport(0, 0, wDst, hDst);
        drawFullscreenQuad();
    }

    /* Upsampling pass */
    bloomUpsample->use();
    bloomUpsample->setUniform("sourceTexture", 0);
    for (int i = mipChainDepth - 2; i >= 0; i--) {
        unsigned int wSrc = width * std::pow(0.5, i + 1);
        unsigned int hSrc = height * std::pow(0.5, i + 1);
        unsigned int wDst = width * std::pow(0.5, i);
        unsigned int hDst = height * std::pow(0.5, i);
        bloomUpsample->setUniform("invRes", glm::vec2(1.0 / wSrc, 1.0 / hSrc));
        bloomUpsample->setUniform("radius", radius);
        bloomUpsample->setUniform("strength", strength);
        glBindTexture(GL_TEXTURE_2D, textures[i + 1]); // input = layer + 1
        glBindFramebuffer(GL_FRAMEBUFFER, FBOS[i]);
        glViewport(0, 0, wDst, hDst);
        drawFullscreenQuad();
    }

    /* Add blur to input image */
    bloomAdd->use();
    bloomAdd->setUniform("tex1", 0);
    bloomAdd->setUniform("tex2", 1);
    glBindTexture(GL_TEXTURE_2D, inputTex); // unit 0
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, textures[0]);
    glViewport(0, 0, this->width, this->height);
    glBindFramebuffer(GL_FRAMEBUFFER, dstFBO);
    drawFullscreenQuad();
    glCheckError();
}
