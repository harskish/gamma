#include "gamma.hpp"
#include "utils.hpp"
#include "GLProgram.hpp"
#include "GLWrappers.hpp"
#include "xxhash.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

unsigned int textureFromFile(std::string path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);
    if (!data) {
        std::cout << "Image loading failed for: " << path << std::endl;
        throw std::runtime_error("Failed to load image " + std::string(path));
    }
    else {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        else
            throw std::runtime_error("Unknown image format");

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    stbi_image_free(data);
    return textureID;
}


// Draw framebuffer texture as overlay for debugging
void showFBTex(GLuint texID, int rows, int cols, int idx) {
    GLProgram* prog = getProgram("Render::FB_TEX", "draw_tex_2d.vert", "draw_tex_2d.frag");
    prog->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    prog->setUniform("fboAttachment", 0);
    drawTexOverlay(rows, cols, idx);
}

// Draw depth texture as overlay for debugging
void showDepthTex(GLuint texID, int rows, int cols, int idx) {
    GLProgram* prog = getProgram("Render::FB_DEPTH", "draw_tex_2d.vert", "draw_depth_2d.frag");
    prog->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    prog->setUniform("depthMap", 0);
    drawTexOverlay(rows, cols, idx);
}

// Indexing starts from bottom left
void drawTexOverlay(int rows, int cols, int idx) {
    struct buffers {
        VertexArray *vao;
        VertexBuffer *vbo;
        VertexBuffer *ebo;
    };

    // One per configuration
    static std::map<std::tuple<int, int, int>, buffers> storage;

    auto key = std::make_tuple(rows, cols, idx);
    auto iter = storage.find(key);
    if (iter == storage.end()) {
        buffers b;
        b.vao = new VertexArray();
        b.vbo = new VertexBuffer();
        b.ebo = new VertexBuffer();

        // NDC: X,Y in [-1,1], positive quadrant north-east
        float w = 2.0f / cols;
        float h = 2.0f / rows;
        float px = (idx % cols) * w - 1.0f;
        float py = (idx / cols) * w - 1.0f;

        const float posTex[16] = {
            px + w, py,     1.0f, 0.0f, // ur
            px,     py,     0.0f, 0.0f, // ul
            px + w, py + h, 1.0f, 1.0f, // br
            px,     py + h, 0.0f, 1.0f  // bl
        };

        const unsigned int inds[6] = {
            0, 1, 2, 2, 1, 3
        };

        b.vao->bind();
        glBindBuffer(GL_ARRAY_BUFFER, b.vbo->id);
        glBufferData(GL_ARRAY_BUFFER, 16 * sizeof(GLfloat), posTex, GL_STATIC_DRAW);
        glCheckError();

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, b.ebo->id);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(unsigned int), inds, GL_STATIC_DRAW);
        glCheckError();

        // ind 0, vec2, offset 0
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(0 * sizeof(GLfloat)));
        glCheckError();

        // ind 1, vec2, offset 2
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void*)(2 * sizeof(GLfloat)));
        glCheckError();

        b.vao->unbind();
        storage[key] = b;
        iter = storage.find(key);
    }

    glDisable(GL_DEPTH_TEST);
    iter->second.vao->bind();
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    iter->second.vao->unbind();
    glEnable(GL_DEPTH_TEST);
}

void applyFilter(GLProgram * prog, GLuint srcTex, GLuint dstTex, GLuint dstFBO) {
    if (srcTex == dstTex) {
        std::cout << "Cannot apply filter in-place" << std::endl;
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, dstFBO);
    if (dstFBO > 0)
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, dstTex, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, srcTex);
    prog->use();
    prog->setUniform("sourceTexture", 0);

    // Draw fullscreen quad using filter
    drawTexOverlay(1, 1, 0);
}

GLProgram * getProgram(std::string tag, std::string vs, std::string fs, map<string, string> repl) {
    return getProgram(tag, vs, "", fs, repl);
}

GLProgram * getProgram(std::string tag, std::string vs, std::string gs, std::string fs, map<string, string> repl) {
    GLProgram* prog = GLProgram::get(tag);
    if (!prog) {
        std::string strVs = (vs != "") ? readShader("Gamma/Shaders/" + vs, repl) : "";
        std::string strGs = (gs != "") ? readShader("Gamma/Shaders/" + gs, repl) : "";
        std::string strFs = (fs != "") ? readShader("Gamma/Shaders/" + fs, repl) : "";
        prog = new GLProgram(strVs, strGs, strFs);
        GLProgram::set(tag, prog);
    }

    return prog;
}

size_t computeHash(const void* buffer, size_t length) {
    size_t seed = 0;
#ifdef ENVIRONMENT64
    size_t const hash = XXH64(buffer, length, seed);
#else
    size_t const hash = XXH32(buffer, length, seed);
#endif
    return hash;
}


// Load file as binary, compute hash
size_t fileHash(const std::string filename) {
    std::ifstream f(filename, std::ios::binary | std::ios::ate);

    if (!f) {
        std::cout << "Could not open file " << filename << " for hashing" << std::endl;
        throw std::runtime_error("Failed to open file " + filename);
    }

    std::ifstream::pos_type pos = f.tellg();
    std::vector<char> data(pos);
    f.seekg(0, std::ios::beg);
    f.read(&data[0], pos);
    f.close();

    return computeHash((const void*)data.data(), (size_t)pos);
}

void drawFullscreenQuad() {
    drawTexOverlay(1, 1, 0);
}

void drawUnitCube() {
    static unsigned int cubeVAO = 0;
    static unsigned int cubeVBO = 0;
    
    if (cubeVAO == 0) {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right         
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
            // front face
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right         
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left     
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right     
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left        
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);

        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    
    // render
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
    glCheckError();
}

