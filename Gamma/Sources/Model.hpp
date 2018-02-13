#pragma once
#include <vector>
#include <string>
#include <assimp/scene.h>
#include "Mesh.hpp"

class Model
{
public:
    Model(std::string path);
    ~Model();

    void render();

private:
    void importMesh(std::string path);
    void recurseNodes(aiNode *node, const aiScene *scene);
    Mesh createMesh(aiMesh * mesh, const aiScene * scene);
    void loadTextures(aiMaterial *mat, aiTextureType type, std::vector<Texture> &target);
    unsigned textureFromFile(const char *path);

    vector<Mesh> meshes;
    vector<Texture> texCache;
    std::string dirPath;
};