#include "Mesh.hpp"
#include "utils.hpp"
#include <glad/glad.h>

Mesh::Mesh(vector<Vertex> &vertices, vector<unsigned int> &indices, vector<Texture> &textures) {
    this->vertices = vertices;
    this->indices = indices;
    this->textures = textures;

    // Generate corresponding OpenGL objects
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO); // all attributes stored in a single buffer (good for static attributes)
    glGenBuffers(1, &EBO);
    glCheckError();

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glCheckError();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glCheckError();

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glCheckError();

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glCheckError();

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    glCheckError();

    glBindVertexArray(0);
}

// Assumes correct program is bound
void Mesh::render() {
    // Set uniforms

    // Set textures

    // Draw
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glCheckError();
    glBindVertexArray(0);
}

Mesh::~Mesh() {
    std::cout << "Mesh destructor called, freeing buffers..." << std::endl;
    /*glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);*/
}


