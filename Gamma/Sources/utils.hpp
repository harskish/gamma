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

// Read shader file, handle includes by recursion
inline std::string readShader(string path, map<string, string> repl = map<string, string>()) {
    path = unixifyPath(path);
    std::ifstream file(path);
    if (!file) {
        std::cout << "Cannot open file " << path << std::endl;
        throw std::runtime_error("Cannot open file " + path);
    }

    size_t idx = path.find_last_of('/');
    std::string dir = path.substr(0, idx);

    std::string output;
    std::string line;

    while (file.good()) {
        getline(file, line);

        if (line.find("#include") == std::string::npos) {
            output.append(line + "\n");
        }
        else {
            std::string includeFileName = line.substr(10, line.length() - 11);
            std::string toAppend = readShader(dir + "/" + includeFileName);
            output.append(toAppend + "\n");
        }
    }

    // Perform string replacements
    for (std::pair<string, string> p : repl) {
        string key = p.first;
        string value = p.second;
        replaceAll(output, key, value);
    }

    return output;
}

// Create GL texture from file
unsigned int textureFromFile(std::string path);

// Draw framebuffer texture as overlay for debugging
void showFBTex(GLuint texID, int rows = 3, int cols = 3, int idx = 2);

// Draw depth texture as overlay for debugging
void showDepthTex(GLuint texID, int rows = 3, int cols = 3, int idx = 2);

// Draw square overlay using given program
// Indexing starts from bottom left
class GLProgram;
void drawTexOverlay(int rows, int cols, int idx);

// Apply filter to src, save into dst
void applyFilter(GLProgram *prog, GLuint srcTex, GLuint dstTex, GLuint dstFBO = 0);

// Helpers for loading programs from shaders
GLProgram* getProgram(std::string tag, std::string vs, std::string fs, map<string, string> repl = map<string, string>());
GLProgram* getProgram(std::string tag, std::string vs, std::string gs, std::string fs, map<string, string> repl = map<string, string>());

// Hashing w/ xxHash
size_t computeHash(const void* buffer, size_t length);
size_t fileHash(const std::string filename);

// Rendering utilities
void drawFullscreenQuad();
void drawUnitCube();