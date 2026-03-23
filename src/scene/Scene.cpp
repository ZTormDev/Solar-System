#include "Scene.hpp"

#include "scene/SphereMesh.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace {
constexpr std::array<std::pair<uint32_t, uint32_t>, 3> kSphereLods = {
    std::pair<uint32_t, uint32_t>{56u, 112u},
    std::pair<uint32_t, uint32_t>{28u, 56u},
    std::pair<uint32_t, uint32_t>{12u, 24u}
};
}

Scene::Scene() {
    buildLodSphereMeshes();
    initializeSolarMvpBodies();
    simulateBodies();
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
    return player.camera();
}

const Camera& Scene::mainCamera() const {
    return player.camera();
}

Player& Scene::mainPlayer() {
    return player;
}

const Player& Scene::mainPlayer() const {
    return player;
}

void Scene::update(GLFWwindow* window, float timeSeconds, float deltaTimeSeconds) {
    elapsedSeconds = timeSeconds;

    if (window != nullptr) {
        player.updateFromInput(window, deltaTimeSeconds);
    }

    if (!paused) {
        elapsedSimulationDays += (static_cast<double>(deltaTimeSeconds) * static_cast<double>(simulationTimeScale)) / REAL_SECONDS_PER_DAY;
    }

    simulateBodies();
}

const std::vector<Vertex>& Scene::meshVertices() const {
    return sceneMeshVertices;
}

const std::vector<uint16_t>& Scene::meshIndices() const {
    return sceneMeshIndices;
}

const Scene::MeshSlice& Scene::meshSliceFor(const GameObject& object) const {
    return lodMeshSlices[object.meshSliceIndex];
}

uint32_t Scene::meshLodCount() const {
    return static_cast<uint32_t>(lodMeshSlices.size());
}

const std::vector<GameObject>& Scene::renderables() const {
    return objects;
}

glm::mat4 Scene::modelMatrixFor(const GameObject& object) const {
    return object.transform.modelMatrix(player.worldPosition());
}

float Scene::timeScale() const {
    return simulationTimeScale;
}

void Scene::setTimeScale(float value) {
    simulationTimeScale = std::clamp(value, 0.0f, 500000.0f);
}

bool Scene::isPaused() const {
    return paused;
}

void Scene::setPaused(bool value) {
    paused = value;
}

double Scene::simulationDays() const {
    return elapsedSimulationDays;
}

const std::vector<CelestialBody>& Scene::celestialBodies() const {
    return bodies;
}

glm::mat4 Scene::viewMatrix() const {
    return player.camera().viewMatrix();
}

glm::mat4 Scene::projectionMatrix(float aspectRatio) const {
    return player.camera().projectionMatrix(aspectRatio);
}

void Scene::buildLodSphereMeshes() {
    lodMeshSlices.clear();
    sceneMeshVertices.clear();
    sceneMeshIndices.clear();

    for (const auto& [latSegments, lonSegments] : kSphereLods) {
        MeshData lodMesh = SphereMesh::createUvSphere(latSegments, lonSegments);

        const uint32_t firstIndex = static_cast<uint32_t>(sceneMeshIndices.size());
        const uint32_t indexCount = static_cast<uint32_t>(lodMesh.indices.size());
        const int32_t vertexOffset = static_cast<int32_t>(sceneMeshVertices.size());

        sceneMeshVertices.insert(sceneMeshVertices.end(), lodMesh.vertices.begin(), lodMesh.vertices.end());

        for (uint16_t index : lodMesh.indices) {
            const uint32_t combinedIndex = static_cast<uint32_t>(index) + static_cast<uint32_t>(vertexOffset);
            sceneMeshIndices.push_back(static_cast<uint16_t>(combinedIndex));
        }

        lodMeshSlices.push_back(MeshSlice{
            .firstIndex = firstIndex,
            .indexCount = indexCount,
            .vertexOffset = 0
        });
    }
}

void Scene::initializeSolarMvpBodies() {
    objects.clear();
    bodies.clear();

    auto addBody = [this](const CelestialBody& bodyTemplate) {
        GameObject& object = createGameObject(bodyTemplate.name);
        object.meshSliceIndex = 1;
        object.transform.scale = glm::vec3(bodyTemplate.visualScale);
        bodies.push_back(bodyTemplate);
    };

    addBody(CelestialBody{
        .name = "Sun",
        .radiusKm = 696340.0,
        .orbitRadiusKm = 0.0,
        .orbitPeriodDays = 0.0,
        .rotationPeriodHours = 609.12,
        .orbitPhaseDegrees = 0.0,
        .visualScale = 2.4f,
        .parentBodyIndex = std::nullopt
    });

    addBody(CelestialBody{
        .name = "Earth",
        .radiusKm = 6371.0,
        .orbitRadiusKm = 149597870.7,
        .orbitPeriodDays = 365.256,
        .rotationPeriodHours = 23.9345,
        .orbitPhaseDegrees = 20.0,
        .visualScale = 0.9f,
        .parentBodyIndex = 0
    });

    addBody(CelestialBody{
        .name = "Moon",
        .radiusKm = 1737.4,
        .orbitRadiusKm = 384400.0,
        .orbitPeriodDays = 27.321661,
        .rotationPeriodHours = 655.72,
        .orbitPhaseDegrees = 35.0,
        .visualScale = 0.35f,
        .parentBodyIndex = 1
    });
}

void Scene::simulateBodies() {
    const double tau = 6.28318530717958647692;

    for (std::size_t index = 0; index < bodies.size(); ++index) {
        const CelestialBody& body = bodies[index];
        GameObject& object = objects[index];

        glm::dvec3 parentPositionUnits = {0.0, 0.0, 0.0};
        if (body.parentBodyIndex.has_value()) {
            parentPositionUnits = objects[body.parentBodyIndex.value()].transform.worldPosition;
        }

        glm::dvec3 orbitOffsetUnits = {0.0, 0.0, 0.0};
        if (body.orbitRadiusKm > 0.0 && body.orbitPeriodDays > 0.0) {
            double orbitRadiusUnits = body.orbitRadiusKm * KM_TO_WORLD_UNITS;
            if (body.parentBodyIndex.has_value()) {
                const GameObject& parentObject = objects[body.parentBodyIndex.value()];
                const double parentVisualRadius = static_cast<double>(parentObject.transform.scale.x);
                const double bodyVisualRadius = static_cast<double>(body.visualScale);
                const double minimumVisualOrbitRadius = (parentVisualRadius + bodyVisualRadius) * 2.4;
                orbitRadiusUnits = std::max(orbitRadiusUnits, minimumVisualOrbitRadius);
            }

            const double angle = glm::radians(body.orbitPhaseDegrees) + tau * (elapsedSimulationDays / body.orbitPeriodDays);
            orbitOffsetUnits.x = std::cos(angle) * orbitRadiusUnits;
            orbitOffsetUnits.y = std::sin(angle) * orbitRadiusUnits;
        }

        object.transform.worldPosition = parentPositionUnits + orbitOffsetUnits;

        const double distanceToCamera = glm::length(object.transform.worldPosition - player.worldPosition());
        object.meshSliceIndex = lodForDistance(distanceToCamera);

        if (body.rotationPeriodHours > 0.0) {
            const double rotationDegrees = wrapDegrees((elapsedSimulationDays * 24.0 / body.rotationPeriodHours) * 360.0);
            object.transform.rotationEulerDegrees = glm::vec3(0.0f, 0.0f, static_cast<float>(rotationDegrees));
        }
    }
}

uint32_t Scene::lodForDistance(double distanceToCameraUnits) const {
    if (distanceToCameraUnits < 8.0) {
        return 0;
    }
    if (distanceToCameraUnits < 35.0) {
        return 1;
    }
    return 2;
}

double Scene::wrapDegrees(double angleDegrees) {
    double wrapped = std::fmod(angleDegrees, 360.0);
    if (wrapped < 0.0) {
        wrapped += 360.0;
    }
    return wrapped;
}
