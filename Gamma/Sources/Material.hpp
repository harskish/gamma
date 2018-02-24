#pragma once
#include <glm/glm.hpp>
#include <assimp/material.h>

// Data needed for GGX shading
typedef struct Material {
    glm::vec3 Kd;
    float metallic;
    float alpha; // shininess (1-roughness)
    unsigned int texMask; // [..., EMISSION, DISP, BUMP, METALLIC, SHININESS, NORMAL, DIFFUSE]

    Material() : Kd(0.6, 0.6, 0.6), metallic(0.5f), alpha(0.5f), texMask(0) {};
} Material;

enum TextureMask {
    DIFFUSE =      (1U << 0),
    NORMAL =       (1U << 1),
    SHININESS =    (1U << 2),
    ROUGHNESS =    (1U << 3),
    METALLIC =     (1U << 4),
    BUMP =         (1U << 5),
    DISPLACEMENT = (1U << 6),
    EMISSION =     (1U << 7)
};

inline TextureMask texTypeToMask(aiTextureType type) {
    switch (type) {
    case aiTextureType_DIFFUSE: return TextureMask::DIFFUSE;
    case aiTextureType_NORMALS: return TextureMask::NORMAL;
    case aiTextureType_SHININESS: return TextureMask::SHININESS;
    case aiTextureType_HEIGHT: return TextureMask::BUMP;
    case aiTextureType_DISPLACEMENT: return TextureMask::DISPLACEMENT;
    case aiTextureType_EMISSIVE: return TextureMask::EMISSION;
    default: return (TextureMask)0U;
    }
}
