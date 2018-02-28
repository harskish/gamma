#include "Mesh.hpp"
#include "utils.hpp"
#include <glad/glad.h>

Mesh::Mesh(vector<Vertex>& vertices, vector<unsigned int>& indices) {
    this->vertices = vertices;
    this->indices = indices;
    this->material = Material();
    init();
}

Mesh::Mesh(vector<Vertex>& vertices, vector<unsigned int>& indices, Material mat) : Mesh(vertices, indices) {
    this->material = mat;
}

Mesh::Mesh(vector<Vertex>& vertices, vector<unsigned int>& indices,
           vector<shared_ptr<Texture>>& textures) : Mesh(vertices, indices) {
    setTextures(textures);
}

Mesh::Mesh(vector<Vertex> &vertices, vector<unsigned int> &indices,
           vector<shared_ptr<Texture>> &textures, Material material) : Mesh(vertices, indices) {
    this->material = material;
    setTextures(textures);
}

Mesh Mesh::Plane(float w, float h) {
    const float x = w / 2.0f;
    const float y = h / 2.0f;

    Vertex ur = { glm::vec3(x, 0.0, y), glm::vec3(0.0, 1.0, 0.0), glm::vec2(0.0, 1.0) };
    Vertex ul = { glm::vec3(-x, 0.0, y), glm::vec3(0.0, 1.0, 0.0), glm::vec2(0.0, 0.0) };
    Vertex br = { glm::vec3(x, 0.0, -y), glm::vec3(0.0, 1.0, 0.0), glm::vec2(1.0, 1.0) };
    Vertex bl = { glm::vec3(-x, 0.0, -y), glm::vec3(0.0, 1.0, 0.0), glm::vec2(1.0, 0.0) };

    std::vector<Vertex> verts = { ur, ul, br, bl };
    std::vector<unsigned int> inds = { 0, 1, 2, 2, 1, 3 };

    return Mesh(verts, inds);
}

void Mesh::init() {
    calculateAABB();

    VAO.reset(new VertexArray());
    VBO.reset(new VertexBuffer());
    EBO.reset(new VertexBuffer());

    VAO->bind();
    glBindBuffer(GL_ARRAY_BUFFER, VBO->id);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
    glCheckError();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO->id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
    glCheckError();

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    glCheckError();

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glCheckError();

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
    glCheckError();

    VAO->unbind();
}

void Mesh::render(GLProgram *prog) {
    // Set uniforms
    prog->setUniform("Kd", material.Kd);
    prog->setUniform("metallic", material.metallic);
    prog->setUniform("shininess", material.alpha);
    prog->setUniform("texMask", material.texMask);
    glCheckError();

    // Set textures
    for (std::shared_ptr<Texture> t : textures) {
        if (t->type == TextureMask::DIFFUSE)
            glActiveTexture(GL_TEXTURE0);
        else if (t->type == TextureMask::NORMAL)
            glActiveTexture(GL_TEXTURE1);
        else if (t->type == TextureMask::SHININESS)
            glActiveTexture(GL_TEXTURE2);
        else if (t->type == TextureMask::ROUGHNESS)
            glActiveTexture(GL_TEXTURE2); // same as shininess
        else if (t->type == TextureMask::METALLIC)
            glActiveTexture(GL_TEXTURE3);
        else
            glActiveTexture(GL_TEXTURE15); // don't overwrite anything!

        glBindTexture(GL_TEXTURE_2D, t->id);
    }
    
    glActiveTexture(GL_TEXTURE0);

    // Draw
    glBindVertexArray(VAO->id);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glCheckError();
    glBindVertexArray(0);
}

// Set textures, update mask
void Mesh::setTextures(vector<shared_ptr<Texture>> v) {
    this->textures = v;
    material.texMask = (TextureMask)0;
    std::for_each(v.begin(), v.end(), [&](shared_ptr<Texture> &t) { material.texMask |= t->type; });
}

// Loads Unreal Engine style PBR textures, available e.g. at freepbr.com
void Mesh::loadPBRTextures(std::string path) {
    std::vector<shared_ptr<Texture>> textures;

    auto add = [&](std::string name, TextureMask mask) {
        try {
            shared_ptr<Texture> tex = std::make_shared<Texture>();
            tex->path = path + "/" + name;
            tex->type = mask;
            tex->id = textureFromFile(tex->path);
            textures.push_back(tex);
        }
        catch (std::runtime_error e) {
            std::cout << "PBR rexture missing: " + name << std::endl;
        }
    };

    add("albedo.png", TextureMask::DIFFUSE);
    add("roughness.png", TextureMask::ROUGHNESS);
    add("normal.png", TextureMask::NORMAL);
    add("metallic.png", TextureMask::METALLIC);

    setTextures(textures);
}

void Mesh::calculateAABB() {
    for (Vertex &v : vertices) {
        aabb.expand(v.position);
    }
}


