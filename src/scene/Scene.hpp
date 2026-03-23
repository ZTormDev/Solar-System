#pragma once

#include "scene/Camera.hpp"
#include "scene/GameObject.hpp"

#include <cstdint>
#include <string>
#include <vector>

class Scene {
public:
    Scene();

    GameObject& createGameObject(const std::string& name);
    std::vector<GameObject>& gameObjects();
    const std::vector<GameObject>& gameObjects() const;

    Camera& mainCamera();
    const Camera& mainCamera() const;

    void update(float timeSeconds);

    const std::vector<Vertex>& activeVertices() const;
    const std::vector<uint16_t>& activeIndices() const;
    uint32_t activeIndexCount() const;
    glm::mat4 activeModelMatrix() const;
    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspectRatio) const;

private:
    const GameObject& activeRenderable() const;
    GameObject& activeRenderable();

    float elapsedSeconds = 0.0f;
    float defaultRotationSpeedDegrees = 45.0f;
    glm::vec3 defaultRotationAxis = {0.3f, 1.0f, 0.0f};

    Camera camera;
    std::vector<GameObject> objects;
};
