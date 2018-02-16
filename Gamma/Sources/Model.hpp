#pragma once
#include <vector>
#include <string>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Mesh.hpp"

class GLProgram;
class Model
{
public:
    Model(std::string path);
    ~Model();

    void render(GLProgram *prog);
    
    glm::mat4 getXform() { return M; }
    void setXform(glm::mat4 m) { M = m; }
    
    // Set material on all meshes
    void setMaterial(Material m);

    // Can be chained
    Model& scale(float s);
    Model& translate(float x, float y, float z);

private:
    void importMesh(std::string path);
    void recurseNodes(aiNode *node, const aiScene *scene);
    Mesh createMesh(aiMesh * mesh, const aiScene * scene);
    void loadTextures(aiMaterial *mat, aiTextureType type, std::vector<Texture> &target);
    unsigned textureFromFile(const char *path);

    glm::mat4 M; // model transform
    vector<Mesh> meshes;
    vector<Texture> texCache;
    std::string dirPath;
};