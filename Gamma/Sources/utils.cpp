#include "utils.hpp"
#include "GLProgram.hpp"
#include "GLWrappers.hpp"

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
    std::string progId = "Render::debugFBTex";
    GLProgram* prog = GLProgram::get(progId);
    if (!prog) {
        prog = new GLProgram(readFile("Gamma/Shaders/draw_tex_2d.vert"),
            readFile("Gamma/Shaders/draw_tex_2d.frag"));
        GLProgram::set(progId, prog);
    }

    prog->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    prog->setUniform("fboAttachment", 0);
    drawTexOverlay(prog, rows, cols, idx);
}

// Draw depth texture as overlay for debugging
void showDepthTex(GLuint texID, int rows, int cols, int idx) {
    std::string progId = "Render::debugFBTex";
    GLProgram* prog = GLProgram::get(progId);
    if (!prog) {
        prog = new GLProgram(readFile("Gamma/Shaders/draw_tex_2d.vert"),
                             readFile("Gamma/Shaders/draw_depth_2d.frag"));
        GLProgram::set(progId, prog);
    }

    prog->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    prog->setUniform("depthMap", 0);
    drawTexOverlay(prog, rows, cols, idx);
}

// Indexing starts from bottom left
void drawTexOverlay(GLProgram * prog, int rows, int cols, int idx) {
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
    glUseProgram(0);
    glEnable(GL_DEPTH_TEST);
}
