#include "Model.hpp"
#include "utils.hpp"
#include <iostream>
#include <algorithm>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

Model::Model(std::string path) {
    this->M = glm::mat4(1.0f);
    importMesh(path);
    calculateAABB();
    texCache.clear();
}

// Assumes correct program is active
void Model::render(GLProgram *prog) {
    prog->setUniform("M", M);
    for (Mesh &m : meshes) {
        m.render(prog);
    }
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
            texture->id = textureFromFile(str.C_Str());
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

unsigned Model::textureFromFile(const char * path) {
    std::string filename = std::string(path);
    filename = this->dirPath + '/' + filename;
    
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
    if (!data) {
        std::cout << "Image loading failed for: " << path << std::endl;
        throw std::runtime_error("Failed to load image " + std::string(path));
    }
    else {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;
        else
            throw std::runtime_error("Unknown image format");

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    }

    stbi_image_free(data);
    return textureID;
}

void Model::calculateAABB() {
    for (Mesh &m : meshes) {
        AABB box = m.getAABB();
        aabb.expand(box);
    }
}
