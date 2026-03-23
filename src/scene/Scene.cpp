#include "Scene.hpp"

#include "scene/CubeMesh.hpp"

#include <stdexcept>

Scene::Scene() {
    GameObject& cube = createGameObject("Cube");
    cube.mesh.vertices = &CubeMesh::vertices();
    cube.mesh.indices = &CubeMesh::indices();
}

GameObject& Scene::createGameObject(const std::string& name) {
    objects.emplace_back(name);
    return objects.back();
}

std::vector<GameObject>& Scene::gameObjects() {
    return objects;
}

const std::vector<GameObject>& Scene::gameObjects() const {
    return objects;
}

Camera& Scene::mainCamera() {
    return camera;
}

const Camera& Scene::mainCamera() const {
    return camera;
}

void Scene::update(float timeSeconds) {
    elapsedSeconds = timeSeconds;

    if (!objects.empty()) {
        GameObject& cube = objects[0];
        cube.transform.rotationEulerDegrees = defaultRotationAxis * (elapsedSeconds * defaultRotationSpeedDegrees);
    }
}

const GameObject& Scene::activeRenderable() const {
    for (const auto& object : objects) {
        if (object.isActive && object.mesh.isValid()) {
            return object;
        }
    }

    throw std::runtime_error("Scene has no active renderable object.");
}

GameObject& Scene::activeRenderable() {
    for (auto& object : objects) {
        if (object.isActive && object.mesh.isValid()) {
            return object;
        }
    }

    throw std::runtime_error("Scene has no active renderable object.");
}

const std::vector<Vertex>& Scene::activeVertices() const {
    return *activeRenderable().mesh.vertices;
}

const std::vector<uint16_t>& Scene::activeIndices() const {
    return *activeRenderable().mesh.indices;
}

uint32_t Scene::activeIndexCount() const {
    return static_cast<uint32_t>(activeIndices().size());
}

glm::mat4 Scene::activeModelMatrix() const {
    return activeRenderable().transform.modelMatrix();
}

glm::mat4 Scene::viewMatrix() const {
    return camera.viewMatrix();
}

glm::mat4 Scene::projectionMatrix(float aspectRatio) const {
    return camera.projectionMatrix(aspectRatio);
}
