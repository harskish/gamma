#pragma once
#include <vector>
#include "Model.hpp"

class Scene {

public:
    Scene() = default;
    ~Scene() = default;

    void addModel(Model &m) { storage.push_back(m); }
    std::vector<Model>& models() { return storage; };

private:
    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    std::vector<Model> storage;
};
