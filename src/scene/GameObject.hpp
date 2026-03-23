#pragma once

#include "renderer/Vertex.hpp"
#include "scene/Transform.hpp"

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

struct MeshReference {
    const std::vector<Vertex>* vertices = nullptr;
    const std::vector<uint16_t>* indices = nullptr;

    bool isValid() const {
        return vertices != nullptr && indices != nullptr;
    }
};

class GameObject {
public:
    explicit GameObject(std::string objectName) : name(std::move(objectName)) {}

    std::string name;
    bool isActive = true;
    Transform transform;
    uint32_t meshSliceIndex = 0;
    glm::vec3 colorTint = glm::vec3(1.0f);
    MeshReference mesh;
};
