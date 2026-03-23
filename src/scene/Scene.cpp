#include "Scene.hpp"

#include "scene/SphereMesh.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace {
constexpr std::array<std::pair<uint32_t, uint32_t>, 5> kSphereLods = {
    std::pair<uint32_t, uint32_t>{128u, 256u},
    std::pair<uint32_t, uint32_t>{96u, 192u},
    std::pair<uint32_t, uint32_t>{72u, 144u},
    std::pair<uint32_t, uint32_t>{48u, 96u},
    std::pair<uint32_t, uint32_t>{32u, 64u}
};
}

Scene::Scene() {
    buildLodSphereMeshes();
    initializeSolarMvpBodies();
    initializeNBodyState();
    syncObjectsFromPhysics();
    updateBodyRotations();
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
        const double simulatedSeconds = static_cast<double>(deltaTimeSeconds) * static_cast<double>(simulationTimeScale);
        const uint32_t substepCount = static_cast<uint32_t>(std::max(
            1.0,
            std::ceil(simulatedSeconds / MAX_PHYSICS_SUBSTEP_SECONDS)
        ));
        const double substepSeconds = simulatedSeconds / static_cast<double>(substepCount);

        for (uint32_t substepIndex = 0; substepIndex < substepCount; ++substepIndex) {
            stepNBody(substepSeconds);
        }

        elapsedSimulationDays += simulatedSeconds / REAL_SECONDS_PER_DAY;
    }

    syncObjectsFromPhysics();
    updateBodyRotations();
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

        sceneMeshIndices.insert(sceneMeshIndices.end(), lodMesh.indices.begin(), lodMesh.indices.end());

        lodMeshSlices.push_back(MeshSlice{
            .firstIndex = firstIndex,
            .indexCount = indexCount,
            .vertexOffset = vertexOffset
        });
    }
}

void Scene::initializeSolarMvpBodies() {
    objects.clear();
    bodies.clear();

    auto addBody = [this](const CelestialBody& bodyTemplate) {
        GameObject& object = createGameObject(bodyTemplate.name);
        object.meshSliceIndex = 1;
        object.transform.scale = glm::vec3(static_cast<float>(bodyTemplate.radiusKm * METERS_PER_KILOMETER));

        if (bodyTemplate.name == "Sun") {
            object.colorTint = glm::vec3(1.8f, 1.35f, 0.6f);
        } else if (bodyTemplate.name == "Earth") {
            object.colorTint = glm::vec3(0.35f, 0.65f, 1.2f);
        } else if (bodyTemplate.name == "Moon") {
            object.colorTint = glm::vec3(0.95f, 0.95f, 1.05f);
        }

        bodies.push_back(bodyTemplate);
    };

    addBody(CelestialBody{
        .name = "Sun",
        .radiusKm = 696340.0,
        .massKg = 1.98847e30,
        .orbitRadiusKm = 0.0,
        .orbitPeriodDays = 0.0,
        .rotationPeriodHours = 609.12,
        .orbitPhaseDegrees = 0.0,
        .parentBodyIndex = std::nullopt
    });

    addBody(CelestialBody{
        .name = "Earth",
        .radiusKm = 6371.0,
        .massKg = 5.97219e24,
        .orbitRadiusKm = 149597870.7,
        .orbitPeriodDays = 365.256,
        .rotationPeriodHours = 23.9345,
        .orbitPhaseDegrees = 20.0,
        .parentBodyIndex = 0
    });

    addBody(CelestialBody{
        .name = "Moon",
        .radiusKm = 1737.4,
        .massKg = 7.342e22,
        .orbitRadiusKm = 384400.0,
        .orbitPeriodDays = 27.321661,
        .rotationPeriodHours = 655.72,
        .orbitPhaseDegrees = 35.0,
        .parentBodyIndex = 1
    });
}

void Scene::initializeNBodyState() {
    bodyPositionsKm.assign(bodies.size(), glm::dvec3(0.0));
    bodyVelocitiesKmPerSec.assign(bodies.size(), glm::dvec3(0.0));

    for (std::size_t index = 0; index < bodies.size(); ++index) {
        const CelestialBody& body = bodies[index];

        if (body.name == "Sun") {
            bodyPositionsKm[index] = glm::dvec3(0.0, 0.0, 0.0);
            bodyVelocitiesKmPerSec[index] = glm::dvec3(0.0, 0.0, 0.0);
            continue;
        }

        if (body.name == "Earth") {
            bodyPositionsKm[index] = glm::dvec3(147098290.0, 0.0, 0.0);
            bodyVelocitiesKmPerSec[index] = glm::dvec3(0.0, 30.29, 0.0);
            continue;
        }

        if (body.name == "Moon") {
            const std::size_t earthIndex = 1;
            bodyPositionsKm[index] = bodyPositionsKm[earthIndex] + glm::dvec3(384400.0, 0.0, 0.0);
            bodyVelocitiesKmPerSec[index] = bodyVelocitiesKmPerSec[earthIndex] + glm::dvec3(0.0, 1.022, 0.0);
            continue;
        }
    }
}

std::vector<glm::dvec3> Scene::computeAccelerations(const std::vector<glm::dvec3>& positionsKm) const {
    std::vector<glm::dvec3> accelerations(bodies.size(), glm::dvec3(0.0));

    for (std::size_t bodyIndex = 0; bodyIndex < bodies.size(); ++bodyIndex) {
        for (std::size_t otherIndex = 0; otherIndex < bodies.size(); ++otherIndex) {
            if (bodyIndex == otherIndex) {
                continue;
            }

            const glm::dvec3 displacementKm = positionsKm[otherIndex] - positionsKm[bodyIndex];
            const double distanceSquaredKm = glm::dot(displacementKm, displacementKm);
            if (distanceSquaredKm < 1.0) {
                continue;
            }

            const double distanceKm = std::sqrt(distanceSquaredKm);
            const glm::dvec3 direction = displacementKm / distanceKm;
            const double accelerationMagnitude = (G_KM3_PER_KG_S2 * bodies[otherIndex].massKg) / distanceSquaredKm;
            accelerations[bodyIndex] += direction * accelerationMagnitude;
        }
    }

    return accelerations;
}

void Scene::stepNBody(double deltaTimeSeconds) {
    if (bodyPositionsKm.empty() || bodyVelocitiesKmPerSec.empty()) {
        return;
    }

    const std::vector<glm::dvec3> initialAccelerations = computeAccelerations(bodyPositionsKm);

    std::vector<glm::dvec3> updatedPositions = bodyPositionsKm;
    for (std::size_t index = 0; index < bodies.size(); ++index) {
        updatedPositions[index] += bodyVelocitiesKmPerSec[index] * deltaTimeSeconds +
                                   0.5 * initialAccelerations[index] * deltaTimeSeconds * deltaTimeSeconds;
    }

    const std::vector<glm::dvec3> updatedAccelerations = computeAccelerations(updatedPositions);

    for (std::size_t index = 0; index < bodies.size(); ++index) {
        bodyVelocitiesKmPerSec[index] += 0.5 * (initialAccelerations[index] + updatedAccelerations[index]) * deltaTimeSeconds;
    }

    bodyPositionsKm = std::move(updatedPositions);
}

void Scene::syncObjectsFromPhysics() {
    for (std::size_t index = 0; index < bodies.size(); ++index) {
        GameObject& object = objects[index];
        object.transform.worldPosition = bodyPositionsKm[index] * METERS_PER_KILOMETER;

        const double distanceToCamera = glm::length(object.transform.worldPosition - player.worldPosition());
        const double bodyRadiusUnits = std::max(1.0, static_cast<double>(object.transform.scale.x));
        object.meshSliceIndex = lodForDistance(distanceToCamera, bodyRadiusUnits);
    }
}

void Scene::updateBodyRotations() {
    for (std::size_t index = 0; index < bodies.size(); ++index) {
        const CelestialBody& body = bodies[index];
        GameObject& object = objects[index];

        if (body.rotationPeriodHours > 0.0) {
            const double rotationDegrees = wrapDegrees((elapsedSimulationDays * 24.0 / body.rotationPeriodHours) * 360.0);
            object.transform.rotationEulerDegrees = glm::vec3(0.0f, 0.0f, static_cast<float>(rotationDegrees));
        }
    }
}

uint32_t Scene::lodForDistance(double distanceToCameraUnits, double bodyRadiusUnits) const {
    const double distanceToRadiusRatio = distanceToCameraUnits / std::max(bodyRadiusUnits, 1.0);

    if (distanceToRadiusRatio < 5.0) {
        return 0;
    }
    if (distanceToRadiusRatio < 10.0) {
        return 1;
    }
    if (distanceToRadiusRatio < 22.0) {
        return 2;
    }
    if (distanceToRadiusRatio < 55.0) {
        return 3;
    }
    return 4;
}

double Scene::wrapDegrees(double angleDegrees) {
    double wrapped = std::fmod(angleDegrees, 360.0);
    if (wrapped < 0.0) {
        wrapped += 360.0;
    }
    return wrapped;
}
