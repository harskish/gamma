#pragma once
#include <float.h>
#include <math.h>
#include <glm/glm.hpp>

class AABB {
public:
    AABB(void) : mins(FLT_MAX), maxs(-FLT_MAX) {};
    AABB(glm::vec3 min, glm::vec3 max) : mins(min), maxs(max) {};

    void expand(glm::vec3 p);
    void expand(AABB &box);
    unsigned int maxDim();

    glm::vec3 mins;
    glm::vec3 maxs;
};
