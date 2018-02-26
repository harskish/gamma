#include "AABB.hpp"

void AABB::expand(glm::vec3 p) {
    mins = glm::min(mins, p);
    maxs = glm::max(maxs, p);
}

void AABB::expand(AABB &box) {
    expand(box.mins);
    expand(box.maxs);
}

unsigned int AABB::maxDim() {
    unsigned int axis = 0;
    glm::vec3 d = maxs - mins;
    if (d.y > d[axis]) axis = 1;
    if (d.z > d[axis]) axis = 2;
    return axis;
}
