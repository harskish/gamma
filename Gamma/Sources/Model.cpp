#include "Model.hpp"
#include "utils.hpp"
#include <iostream>
#include <algorithm>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

Model::Model(std::string path) : M(1.0f) {
    importMesh(path);
    calculateAABB();
    texCache.clear();
}

Model::Model(Mesh & m) : M(1.0f) {
    addMesh(m);
}

// Assumes correct program is active
void Model::render(GLProgram *prog) {
    prog->setUniform("M", M);
    for (Mesh &m : meshes) {
        m.setupGGXParams(prog);
        m.render(prog);
    }
}

// For shadow map depth pass
void Model::renderUnshaded(GLProgram *prog) {
    prog->setUniform("M", M);
    for (Mesh &m : meshes) {
        m.render(prog);
    }
}

void Model::normalizeScale() {
    unsigned int i = aabb.maxDim();
    glm::vec3 d = aabb.maxs - aabb.mins;
    scale(1.0f / d[i]);
}

void Model::addMesh(Mesh & m) {
    meshes.push_back(m);
    calculateAABB();
}

void Model::setMaterial(Material m) {
    for (Mesh &mesh : meshes) {
        mesh.setMaterial(m);
    }
}

// Used for looping through all materials in model
std::vector<Material*> Model::getMaterials() {
    std::vector<Material*> materials;
    for (Mesh &m : meshes) {
        materials.push_back(&m.getMaterial());
    }
    return materials;
}

Model& Model::scale(float s) {
    setXform(glm::scale(M, glm::vec3(s)));
    return *this;
}

Model& Model::translate(float x, float y, float z) {
    setXform(glm::translate(M, glm::vec3(x, y, z)));
    return *this;
}

void Model::importMesh(std::string path) {
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        std::cout << "ASSIMP error: " << importer.GetErrorString() << std::endl;
        throw std::runtime_error("Model loading failed!");
    }

    // Save directory for texture loading
    dirPath = path.substr(0, path.find_last_of('/'));
    recurseNodes(scene->mRootNode, scene);
}

// Recursively process current node and its children
void Model::recurseNodes(aiNode * node, const aiScene * scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.push_back(createMesh(mesh, scene));
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        recurseNodes(node->mChildren[i], scene);
    }
}

// Create mesh object from an aiMesh struct
Mesh Model::createMesh(aiMesh * mesh, const aiScene * scene) {
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<shared_ptr<Texture>> textures;
    
    auto spread = [](glm::vec3 &v1, aiVector3D &v2) {
        for (int i = 0; i < 3; i++) v1[i] = v2[i];
    };
    
    // Fill vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        Vertex v;
        spread(v.position, mesh->mVertices[i]);
        spread(v.normal, mesh->mNormals[i]);
        
        v.texCoords = glm::vec2(0.0f, 0.0f);
        if (mesh->mTextureCoords[0]) {
            v.texCoords.x = mesh->mTextureCoords[0][i].x;
            v.texCoords.y = mesh->mTextureCoords[0][i].y;
        }

        vertices.push_back(v);
    }

    // Fill indices
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    // Material
    aiMaterial* material = scene->mMaterials[mesh->mMaterialIndex];
    Material mat = Material();
    material->Get(AI_MATKEY_COLOR_DIFFUSE, mat.Kd);
    material->Get(AI_MATKEY_SHININESS, mat.alpha);
    loadTextures(material, aiTextureType_DIFFUSE, textures);
    loadTextures(material, aiTextureType_NORMALS, textures);
    loadTextures(material, aiTextureType_SHININESS, textures);
    std::for_each(textures.begin(), textures.end(), [&mat](shared_ptr<Texture> &t) { mat.texMask |= t->type; });

    return Mesh(vertices, indices, textures, mat);
}

void Model::loadTextures(aiMaterial *mat, aiTextureType type, std::vector<shared_ptr<Texture>>& target) {
    unsigned int count = mat->GetTextureCount(type);

    if (count > 1)
        std::cout << "WARN: multiple textures of type " << texTypeToName(type) << "in mesh" << std::endl;

    for (unsigned int i = 0; i < count; i++) {
        aiString str;
        mat->GetTexture(type, i, &str);
       
        // Check for duplicates
        auto match = std::find_if(texCache.begin(), texCache.end(), [str](shared_ptr<Texture> &t) {
            return !std::strcmp(t->path.data(), str.C_Str()); // strcmp returns 0 for match
        });

        if (match == texCache.end()) {
            shared_ptr<Texture> texture = std::make_shared<Texture>();
            texture->id = textureFromFile(this->dirPath + '/' + str.C_Str());
            texture->type = texTypeToMask(type);
            texture->path = str.C_Str();
            target.push_back(texture);
            texCache.push_back(texture);
        }
        else {
            target.push_back(*match);
        }
    }
}

void Model::calculateAABB() {
    for (Mesh &m : meshes) {
        AABB box = m.getAABB();
        aabb.expand(box);
    }
}
