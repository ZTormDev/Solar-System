#pragma once

#include "scene/Camera.hpp"
#include "scene/CelestialBody.hpp"
#include "scene/GameObject.hpp"
#include "scene/Player.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

class Scene {
public:
    struct MeshSlice {
        uint32_t firstIndex = 0;
        uint32_t indexCount = 0;
        int32_t vertexOffset = 0;
    };

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
    const MeshSlice& meshSliceFor(const GameObject& object) const;
    uint32_t meshLodCount() const;

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
    void buildLodSphereMeshes();
    void initializeSolarMvpBodies();
    void initializeNBodyState();
    void stepNBody(double deltaTimeSeconds);
    std::vector<glm::dvec3> computeAccelerations(const std::vector<glm::dvec3>& positionsKm) const;
    void syncObjectsFromPhysics();
    void updateBodyRotations();
    uint32_t lodForDistance(double distanceToCameraUnits, double bodyRadiusUnits) const;
    static double wrapDegrees(double angleDegrees);

    float elapsedSeconds = 0.0f;
    double elapsedSimulationDays = 0.0;
    float simulationTimeScale = 6000.0f;
    bool paused = false;

    static constexpr double REAL_SECONDS_PER_DAY = 86400.0;
    static constexpr double METERS_PER_KILOMETER = 1000.0;
    static constexpr double G_KM3_PER_KG_S2 = 6.67430e-20;
    static constexpr double MAX_PHYSICS_SUBSTEP_SECONDS = 600.0;

    std::vector<CelestialBody> bodies;
    std::vector<MeshSlice> lodMeshSlices;
    std::vector<Vertex> sceneMeshVertices;
    std::vector<uint16_t> sceneMeshIndices;
    std::vector<glm::dvec3> bodyPositionsKm;
    std::vector<glm::dvec3> bodyVelocitiesKmPerSec;

    Player player;
    std::vector<GameObject> objects;
};
