#pragma once
#include <glad/glad.h>
#include <string>
#include <iostream>
#include <assimp/material.h>

// From: https://learnopengl.com/In-Practice/Debugging
inline GLenum glCheckError_(const char *file, int line) {
    GLenum errorCode;
    while ((errorCode = glGetError()) != GL_NO_ERROR) {
        std::string error;
        switch (errorCode) {
        case GL_INVALID_ENUM:                  error = "INVALID_ENUM"; break;
        case GL_INVALID_VALUE:                 error = "INVALID_VALUE"; break;
        case GL_INVALID_OPERATION:             error = "INVALID_OPERATION"; break;
        case GL_STACK_OVERFLOW:                error = "STACK_OVERFLOW"; break;
        case GL_STACK_UNDERFLOW:               error = "STACK_UNDERFLOW"; break;
        case GL_OUT_OF_MEMORY:                 error = "OUT_OF_MEMORY"; break;
        case GL_INVALID_FRAMEBUFFER_OPERATION: error = "INVALID_FRAMEBUFFER_OPERATION"; break;
        }
        std::cout << error << " | " << file << " (" << line << ")" << std::endl;
    }
    return errorCode;
}
#define glCheckError() glCheckError_(__FILE__, __LINE__) 

inline std::string texTypeToName(aiTextureType type) {
    switch (type) {
    case aiTextureType_AMBIENT: return "tex_ambient";
    case aiTextureType_DIFFUSE: return "tex_diffuse";
    case aiTextureType_DISPLACEMENT: return "tex_displacement";
    case aiTextureType_EMISSIVE: return "tex_emissive";
    case aiTextureType_HEIGHT: return "tex_height";
    case aiTextureType_LIGHTMAP: return "tex_lightmap";
    case aiTextureType_NONE: return "tex_none";
    case aiTextureType_NORMALS: return "tex_normals";
    case aiTextureType_OPACITY: return "tex_opacity";
    case aiTextureType_REFLECTION: return "tex_reflection";
    case aiTextureType_SHININESS: return "tex_shininess";
    case aiTextureType_SPECULAR: return "tex_specular";
    case aiTextureType_UNKNOWN: return "tex_unknown";
    default: return "tex_unknown";
    }
}