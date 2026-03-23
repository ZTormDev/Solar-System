#pragma once

#include "scene/Camera.hpp"
#include "scene/CelestialBody.hpp"
#include "scene/GameObject.hpp"
#include "scene/Player.hpp"

#include <cstdint>
#include <string>
#include <vector>

class Scene {
public:
    Scene();

    GameObject& createGameObject(const std::string& name);
    std::vector<GameObject>& gameObjects();
    const std::vector<GameObject>& gameObjects() const;

    Player& mainPlayer();
    const Player& mainPlayer() const;

    Camera& mainCamera();
    const Camera& mainCamera() const;

    void update(GLFWwindow* window, float timeSeconds, float deltaTimeSeconds);

    const std::vector<Vertex>& meshVertices() const;
    const std::vector<uint16_t>& meshIndices() const;
    uint32_t meshIndexCount() const;

    const std::vector<GameObject>& renderables() const;
    glm::mat4 modelMatrixFor(const GameObject& object) const;

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspectRatio) const;

    float timeScale() const;
    void setTimeScale(float value);
    bool isPaused() const;
    void setPaused(bool value);
    double simulationDays() const;

    const std::vector<CelestialBody>& celestialBodies() const;

private:
    void initializeSolarMvpBodies();
    void simulateBodies();
    static double wrapDegrees(double angleDegrees);

    float elapsedSeconds = 0.0f;
    double elapsedSimulationDays = 0.0;
    float simulationTimeScale = 6000.0f;
    bool paused = false;

    static constexpr double REAL_SECONDS_PER_DAY = 86400.0;
    static constexpr double KM_TO_WORLD_UNITS = 1.0 / 1000000.0;

    std::vector<CelestialBody> bodies;

    Player player;
    std::vector<GameObject> objects;
};
