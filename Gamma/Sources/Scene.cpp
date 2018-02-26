#include "Scene.hpp"

void Scene::setMaxLights(size_t max) {
    MAX_LIGHTS = max;
    if (mLights.size() > MAX_LIGHTS) {
        std::cout << "Too many lights in scene, resizing to " << MAX_LIGHTS << std::endl;
        mLights.resize(MAX_LIGHTS);
    }
}

void Scene::addLight(Light & l) {
    if (mLights.size() >= MAX_LIGHTS) {
        std::cout << "Light array full, cannot add new light" << std::endl;
        return;
    }
    
    // Lights sorted according to type for thread coherence in shaders
    mLights.push_back(l);
    std::sort(mLights.begin(), mLights.end(), [](Light &l1, Light &l2) { return l1.vector.w < l2.vector.w; });
}
