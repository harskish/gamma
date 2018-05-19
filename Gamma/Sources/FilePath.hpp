#pragma once
#include <string>
#include "String.hpp"

/* Represents the path to a file on the disk */

class FilePath {
public:
    FilePath(const char* str) {
        path = gma::String(str).asUnixPath();
    }

    std::string fullPath() {
        return path;
    }

    std::string fileName() {
        size_t fileNameStart = path.s_str().find_last_of("/");
        return path.substr(fileNameStart + 1);
    }

    std::string folderPath() {
        size_t fileNameStart = path.s_str().find_last_of("/");
        return path.s_str().substr(0, fileNameStart + 1);
    }

private:
    gma::String path;

};