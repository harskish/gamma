#pragma once
#include <glad/glad.h>

/* 
    A heirarchical bloom filter bsed on the presentation:
    
    "Next Generation Post Processing in Call of Duty Advanced Warfare" by Jorge Jimenez
    http://www.iryoku.com/downloads/Next-Generation-Post-Processing-in-Call-of-Duty-Advanced-Warfare-v18.pptx

    Builds a pyramid of successively smaller images (mip chain). Uses hand-crafted 13-tap bilinear filter
    for downsampling, 3x3 tent filter for upsampling. The progressive blurs converge to a gaussian.

    Also uses partial Karis average on mip0->mip1 transition to combat specular aliasing (due to HDR).
*/


class BloomPass
{
public:
    BloomPass(void) {};
    BloomPass(int width, int height);
    
    BloomPass(const BloomPass&) = delete;
    BloomPass& operator=(BloomPass const&) = delete;
    
    // Handle FB resize
    void resize(int w, int h);

    ~BloomPass(void) {
        freeGLData();
    }

    // Apply bloom filter
    void apply(GLuint inputTex, GLuint dstFBO);

    // Parameters
    float threshold = 1.8f;
    float radius = 1.0f;
    float strength = 0.7f;
    bool useAA = true; // Apply Karis avg at first DS?

private:
    void freeGLData() {
        glDeleteTextures(mipChainDepth, textures);
        glDeleteFramebuffers(mipChainDepth, FBOS);
    }

    int mipChainDepth = 0;
    int width;
    int height;

    // MIP levels as separate FBOs
    GLuint FBOS[16] = {0};
    GLuint textures[16] = {0};
};
