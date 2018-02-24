#pragma once
#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "Material.hpp"
#include "GLProgram.hpp"
#include "AABB.hpp"

using std::vector;
using std::shared_ptr;

typedef struct {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
} Vertex;

class Texture {
public:
    Texture(void) :
        id(0),
        type((TextureMask)0),
        path("")
    {};
    ~Texture() {
        std::cout << "Freeing GL texture for " << path << std::endl;
        glDeleteTextures(1, &id);
    }

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

    unsigned int id;
    TextureMask type;
    std::string path; // for detecting duplicates
};

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

    unsigned int id;
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

    unsigned int id;
};

class Mesh
{
public:
    Mesh(vector<Vertex> &vertices, vector<unsigned int> &indices, vector<shared_ptr<Texture>> &textures, Material mat = Material());
    ~Mesh() = default;
    
    void render(GLProgram *prog);
    void setMaterial(Material m) { material = m; };
    Material& getMaterial() { return material; };
    void setTextures(vector<shared_ptr<Texture>> v);
    AABB getAABB() { return aabb; }

private:
    void calculateAABB();
    
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<shared_ptr<Texture>> textures; // shared among meshes

    AABB aabb;
    Material material;
    shared_ptr<VertexArray> VAO;
    shared_ptr<VertexBuffer> VBO;
    shared_ptr<VertexBuffer> EBO;
    
};
