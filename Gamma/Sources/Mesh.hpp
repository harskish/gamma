#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "Material.hpp"
#include "GLProgram.hpp"

using std::vector;

typedef struct {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
} Vertex;

typedef struct {
    unsigned int id;
    TextureMask type;
    std::string path; // for detecting duplicates
} Texture;

class Mesh
{
public:
    Mesh(vector<Vertex> &vertices, vector<unsigned int> &indices, vector<Texture> &textures, Material mat = Material());
    ~Mesh();
    
    void render(GLProgram *prog);
    void setMaterial(Material m) { material = m; };

private:
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;

    Material material;
    unsigned int VAO, VBO, EBO;
    
};
