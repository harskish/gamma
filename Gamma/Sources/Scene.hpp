#pragma once
#include <vector>
#include "Model.hpp"
#include "Light.hpp"

class Scene {

public:
    Scene() = default;
    ~Scene();

    void addModel(Model &m) { mModels.push_back(m); }
    void clearModels() { mModels.clear(); }
    std::vector<Model>& models() { return mModels; };

    void setMaxLights(size_t max);
    void addLight(Light* l);
    void clearLights();
    std::vector<Light*> &lights() { return mLights; }

private:
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    std::vector<Model> mModels;
    std::vector<Light*> mLights;

    size_t MAX_LIGHTS = 1000; // set by renderer
};
