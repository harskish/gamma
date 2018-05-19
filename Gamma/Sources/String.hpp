#pragma once

#include <string>
#include <vector>
#include <sstream>

/* C++ header-only string library */

namespace gma {
    
    class String {
    public:
        String(void) = default;
        
        String(const char* s) {
            str = std::string(s);
        }

        String(const std::string &s) {
            str = s;
        }

        // Implicit conversion
        operator std::string() const { return str; }
        operator const char*() const { return str.c_str(); }

        // Convert to std::string
        std::string s_str(void) const {
            return str;
        }

        // Convert to c style string
        const char* c_str(void) const {
            return str.c_str();
        }

        char& operator[](const size_t idx) {
            return str[idx];
        }

        bool operator==(const std::string& s) {
            return str == s;
        }

        bool operator==(const char* s) {
            return str == s;
        }

        bool operator==(const String& s) {
            return str == s.s_str();
        }

        String substr(size_t offset, size_t count = std::string::npos) {
            return String(str.substr(offset, count));
        }

        // Split string by character, return parts
        std::vector<std::string> split(const char c) const {
            std::stringstream stream(str);
            std::string segment;
            std::vector<std::string> parts;

            while (std::getline(stream, segment, c)) {
                parts.push_back(segment);
            }

            return parts;
        }

        bool contains(const std::string &s) const {
            return (str.find(s) != std::string::npos);
        }

        bool endsWith(const std::string &end) const {
            size_t len = end.size();
            if (len > str.size()) return false;

            std::string substr = str.substr(str.size() - len, len);
            return end == substr;
        }

        // Replace all occurrances
        void replace(const std::string& from, const std::string& to) {
            size_t startPos = 0;
            while ((startPos = str.find(from, startPos)) != std::string::npos) {
                str.replace(startPos, from.length(), to);
                startPos += to.length();
            }
        }

        // Convert contents to unix-style path
        String asUnixPath(void) const {
            String copy(str);
            copy.replace("\\", "/");
            return copy;
        }

        /*String asUnixPath2(void) const {
            std::string s = str;
            
            size_t index = 0;
            while (true) {
                index = s.find("\\", index);
                if (index == std::string::npos) break;

                s.replace(index, 1, "/");
                index += 1;
            }

            return String(s);
        }*/


    private:
        std::string str; // internal storage

    };

}