#pragma once
#include <glm/glm.hpp>

typedef struct Light {
    glm::vec4 vector; // w=0 => directional, w!=0 => positional
    glm::vec3 emission;
    Light(void) : vector(0.0, 0.0, 0.0, 1.0), emission(5.0) {};
    Light(glm::vec4 v, glm::vec3 e) : vector(v), emission(e) {};
} Light;