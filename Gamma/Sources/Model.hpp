#pragma once
#include <vector>
#include <string>
#include <memory>
#include <assimp/scene.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "AABB.hpp"
#include "Mesh.hpp"

using std::shared_ptr;

class GLProgram;
class Model
{
public:
    Model(std::string path);
    Model(Mesh &m);
    Model(void) {};
    ~Model() = default;

    void render(GLProgram *prog);
    void renderUnshaded(GLProgram *prog);
    
    glm::mat4 getXform() { return M; }
    void setXform(glm::mat4 m) { M = m; }
    void normalizeScale();

    Mesh& getMesh(unsigned int ind) { return meshes[ind]; }
    void addMesh(Mesh &m);
    
    // Set material on all meshes
    void setMaterial(Material m);
    std::vector<Material*> getMaterials();

    // Can be chained
    Model& scale(float s);
    Model& translate(float x, float y, float z);

private:
    void importMesh(std::string path);
    void recurseNodes(aiNode *node, const aiScene *scene);
    Mesh createMesh(aiMesh * mesh, const aiScene * scene);
    void loadTextures(aiMaterial *mat, aiTextureType type, std::vector<shared_ptr<Texture>> &target);
    void calculateAABB();

    AABB aabb;
    glm::mat4 M; // model transform
    vector<Mesh> meshes;
    vector<shared_ptr<Texture>> texCache;
    std::string dirPath;
};