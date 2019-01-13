#pragma once
#include <vector>
#include <string>
#include "Model.hpp"
#include "Light.hpp"
#include "IBLMaps.hpp"
#include "ParticleSystem.hpp"

using std::string;

class Scene {

public:
    Scene(const char* scenefile);
    Scene() { iblMaps.reset(new IBLMaps()); };
    ~Scene();

    void initFromFile(const char* scenefile);
    void addModel(Model &m) { mModels.push_back(m); }
    void clearModels() { mModels.clear(); }
    std::vector<Model>& models() { return mModels; };
    void addParticleSystem(const shared_ptr<ParticleSystem>& sys) { mParticleSystems.push_back(sys); }
    std::vector<shared_ptr<ParticleSystem>>& particleSystems() { return mParticleSystems; };

    void setMaxLights(size_t max);
    void addLight(Light* l);
    void clearLights();
    std::vector<Light*> &lights() { return mLights; }

    void loadIBLMaps(std::string name, float brightness = 1.0f);
    std::shared_ptr<IBLMaps> getIBLMaps() { return iblMaps; }

private:
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    std::vector<Model> mModels;
    std::vector<Light*> mLights;
    std::vector<shared_ptr<ParticleSystem>> mParticleSystems; // ptr to avoid object slicing
    std::shared_ptr<IBLMaps> iblMaps;

    size_t MAX_LIGHTS = 1000; // set by renderer
};
