#pragma once
#include <vector>
#include <string>
#include <memory>
#include <map>
#include <glm/glm.hpp>
#include "Material.hpp"
#include "GLProgram.hpp"
#include "AABB.hpp"
#include "GLWrappers.hpp"

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
    Mesh(vector<Vertex> &vertices, vector<unsigned int> &indices);
    Mesh(vector<Vertex> &vertices, vector<unsigned int> &indices, Material mat);
    Mesh(vector<Vertex> &vertices, vector<unsigned int> &indices, vector<shared_ptr<Texture>> &textures);
    Mesh(vector<Vertex> &vertices, vector<unsigned int> &indices, vector<shared_ptr<Texture>> &textures, Material mat);
    ~Mesh() = default;
    
    void setupGGXParams(GLProgram *prog);
    void render(GLProgram *prog);

    void setMaterial(Material m) { material = m; };
    Material& getMaterial() { return material; };
    void setTextures(vector<shared_ptr<Texture>> v);
    void loadPBRTextures(std::map<std::string, std::string> &paths);
    void loadPBRTextures(std::string path);
    AABB getAABB() { return aabb; }

    // Mesh generators
    static Mesh Plane(float w, float h);

private:
    void init();
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
