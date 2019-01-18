#pragma once
#include <glad/glad.h>
#include <iostream>

class VertexArray {
public:
    VertexArray(void) : id(0) {
        glGenVertexArrays(1, &id);
    }

    ~VertexArray() {
        std::cout << "Freeing vertex array " << id << std::endl;
        glDeleteVertexArrays(1, &id);
    }

    void bind() { glBindVertexArray(id); }
    void unbind() { glBindVertexArray(0); }

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;

    GLuint id;
};

class VertexBuffer {
public:
    VertexBuffer(void) : id(0) {
        glGenBuffers(1, &id);
    }

    ~VertexBuffer() {
        std::cout << "Freeing vertex buffer " << id << std::endl;
        glDeleteBuffers(1, &id);
    }

    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;

    GLuint id;
};
