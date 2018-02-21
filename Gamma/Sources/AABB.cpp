#include "AABB.hpp"

void AABB::expand(glm::vec3 p) {
    mins = glm::min(mins, p);
    maxs = glm::max(maxs, p);
}

void AABB::expand(AABB &box) {
    expand(box.mins);
    expand(box.maxs);
}
