#pragma once
#include <vector>
#include <string>
#include <glm/glm.hpp>

using std::vector;

typedef struct {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
} Vertex;

typedef struct {
    unsigned int id;
    std::string type; // for creating sampler names programmatically
    std::string path; // for detecting duplicates
} Texture;

class Mesh
{
public:
    Mesh(vector<Vertex> &vertices, vector<unsigned int> &indices, vector<Texture> &textures);
    ~Mesh();
    
    void render();

private:
    vector<Vertex> vertices;
    vector<unsigned int> indices;
    vector<Texture> textures;

    unsigned int VAO, VBO, EBO;
    
};
