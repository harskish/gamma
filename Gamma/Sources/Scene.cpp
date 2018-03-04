#include "Scene.hpp"

Scene::~Scene() {
    for (Light *l : mLights) {
        delete l;
    }
}

void Scene::setMaxLights(size_t max) {
    MAX_LIGHTS = max;

    if (mLights.size() > MAX_LIGHTS) {
        std::cout << "Too many lights in scene, resizing to " << MAX_LIGHTS << std::endl;
        for (int i = MAX_LIGHTS; i < mLights.size(); i++) {
            delete mLights[i];
        }
        mLights.resize(MAX_LIGHTS);
    }
}

// Takes ownership
void Scene::addLight(Light* l) {
    if (mLights.size() >= MAX_LIGHTS) {
        std::cout << "Light array full, cannot add new light" << std::endl;
        return;
    }
    
    // Lights sorted according to type for thread coherence in shaders
    mLights.push_back(l);
    std::sort(mLights.begin(), mLights.end(), [](Light *l1, Light *l2) { return l1->vector.w < l2->vector.w; });
}

void Scene::clearLights() {
    for (Light* l : mLights) {
        delete l;
    }
    mLights.clear();
}
