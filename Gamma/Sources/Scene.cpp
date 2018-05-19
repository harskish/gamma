#include "Scene.hpp"
#include "String.hpp"
#include "FilePath.hpp"
#include <map>

Scene::Scene(const char * scenefile) : Scene() {
    initFromFile(scenefile);
}

Scene::~Scene() {
    for (Light *l : mLights) {
        delete l;
    }
}

void Scene::initFromFile(const char * scenefile) {
    FilePath path(scenefile);
    std::string folder = path.folderPath();
    
    std::ifstream specs(path.fullPath());
    if (!specs.good()) {
        std::cout << "Could not open scenefile '" << scenefile << "'" << std::endl;
        return;
    }

    Model *model = nullptr; // model currently being processed (scenefile can contain multiple)
    std::map<std::string, std::string> texPaths;
    
    // Currnet model processed, add to scene
    auto pushModel = [&]() {
        if (model) {
            model->loadPBRTextures(texPaths);
            addModel(*model); // makes copy
            texPaths.clear();
            delete model;
        }
    };
    
    std::string line;
    while (getline(specs, line)) {
        auto parts = gma::String(line).split(' ');
        std::string key = parts[0];
        if (key == "geometry") {
            pushModel();
            model = new Model(folder + parts[1]);
        }
        if (key == "albedo") {
            texPaths["albedo"] = folder + parts[1];
        }
        if (key == "roughness") {
            texPaths["roughness"] = folder + parts[1];
        }
        if (key == "normal") {
            texPaths["normal"] = folder + parts[1];
        }
        if (key == "metallic") {
            texPaths["metallic"] = folder + parts[1];
        }
        if (key == "environment") {
            loadIBLMaps(folder + parts[1]);
        }
    }
    pushModel();
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

void Scene::loadIBLMaps(std::string name) {
    iblMaps.reset(new IBLMaps(name));
}
