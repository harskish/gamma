#pragma once
#include <glad/glad.h>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>
#include <assimp/material.h>

using std::map;
using std::string;

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

inline bool endsWith(const string &s, const string &end) {
    size_t len = end.size();
    if (len > s.size()) return false;

    string substr = s.substr(s.size() - len, len);
    return end == substr;
}

inline std::string unixifyPath(string path) {
    size_t index = 0;
    while (true) {
        index = path.find("\\", index);
        if (index == string::npos) break;

        path.replace(index, 1, "/");
        index += 1;
    }

    return path;
}

inline void replaceAll(string &str, const string& from, const string& to) {
    size_t startPos = 0;
    while ((startPos = str.find(from, startPos)) != std::string::npos) {
        str.replace(startPos, from.length(), to);
        startPos += to.length();
    }
}

inline std::string readFile(string path, map<string, string> repl = map<string, string>()) {
    std::ifstream stream(path);
    if (!stream) {
        std::cout << "Cannot open file " << path << std::endl;
        throw std::runtime_error("Cannot open file " + path);
    }

    std::stringstream buffer;
    buffer << stream.rdbuf();
    string str = buffer.str();

    // Perform string replacements
    for (std::pair<string, string> p : repl) {
        string key = p.first;
        string value = p.second;
        replaceAll(str, key, value);
    }

    return str;
}

// Create GL texture from file
unsigned int textureFromFile(std::string path);