#pragma once
#include <vector>
#include <string>
#include <memory>
#include <glm/glm.hpp>
#include "Material.hpp"
#include "GLProgram.hpp"

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

class Mesh
{
public:
    Mesh(vector<Vertex> &vertices, vector<unsigned int> &indices, vector<shared_ptr<Texture>> &textures, Material mat = Material());
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    
    void render(GLProgram *prog);
    void setMaterial(Material m) { material = m; };

private:
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<shared_ptr<Texture>> textures; // shared among meshes

    Material material;
    unsigned int VAO = 0, VBO = 0, EBO = 0;
    
};
