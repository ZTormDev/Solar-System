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

    struct EphemerisValidationEntry {
        std::string bodyName;
        double referenceJulianDay = 2451545.0;
        double errorKm = 0.0;
    };

    Scene();

    GameObject& createGameObject(const std::string& name);
    std::vector<GameObject>& gameObjects();
    const std::vector<GameObject>& gameObjects() const;

    Player& mainPlayer();
    const Player& mainPlayer() const;

    Camera& mainCamera();
    const Camera& mainCamera() const;

    void update(SDL_Window* window, float timeSeconds, float deltaTimeSeconds);

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
    double currentJulianDay() const;

    const std::vector<CelestialBody>& celestialBodies() const;
    const std::vector<EphemerisValidationEntry>& ephemerisValidationEntries() const;
    glm::dvec3 sunWorldPosition() const;
    void followBody(std::size_t bodyIndex);
    void clearFollowBody();
    std::optional<std::size_t> followedBodyIndex() const;

private:
    void buildLodSphereMeshes();
    void initializeSolarMvpBodies();
    void initializePhysicsState();
    void stepNBody(double deltaTimeSeconds);
    std::vector<glm::dvec3> computeAccelerations(const std::vector<glm::dvec3>& positionsKm) const;
    void syncObjectsFromPhysics();
    void updateBodyRotations();
    void updateValidationReport();
    std::pair<glm::dvec3, glm::dvec3> orbitalStateFromElements(const OrbitalElementsJ2000& elements, double julianDay) const;
    glm::dvec3 heliocentricPositionFromElements(const OrbitalElementsJ2000& elements, double julianDay) const;
    static double solveEccentricAnomalyRad(double meanAnomalyRad, double eccentricity);
    uint32_t lodForDistance(double distanceToCameraUnits, double bodyRadiusUnits) const;
    static double wrapDegrees(double angleDegrees);
    static double wrapRadians(double angleRadians);

    float elapsedSeconds = 0.0f;
    double elapsedSimulationDays = 0.0;
    float simulationTimeScale = 6000.0f;
    bool paused = false;

    static constexpr double J2000_JULIAN_DAY = 2451545.0;
    static constexpr double REAL_SECONDS_PER_DAY = 86400.0;
    static constexpr double METERS_PER_KILOMETER = 1000.0;
    static constexpr double G_KM3_PER_KG_S2 = 6.67430e-20;
    static constexpr double MAX_PHYSICS_SUBSTEP_SECONDS = 300.0;

    std::vector<CelestialBody> bodies;
    std::vector<MeshSlice> lodMeshSlices;
    std::vector<Vertex> sceneMeshVertices;
    std::vector<uint16_t> sceneMeshIndices;
    std::vector<glm::dvec3> bodyHeliocentricPositionsKm;
    std::vector<glm::dvec3> bodyHeliocentricVelocitiesKmPerSec;
    std::vector<EphemerisValidationEntry> validationEntries;
    std::optional<std::size_t> currentFollowedBodyIndex;

    Player player;
    std::vector<GameObject> objects;
};
