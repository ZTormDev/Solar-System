#include "Scene.hpp"

#include "scene/CubeMesh.hpp"

#include <algorithm>
#include <cmath>

Scene::Scene() {
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
    return CubeMesh::vertices();
}

const std::vector<uint16_t>& Scene::meshIndices() const {
    return CubeMesh::indices();
}

uint32_t Scene::meshIndexCount() const {
    return static_cast<uint32_t>(meshIndices().size());
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

void Scene::initializeSolarMvpBodies() {
    objects.clear();
    bodies.clear();

    auto addBody = [this](const CelestialBody& bodyTemplate) {
        GameObject& object = createGameObject(bodyTemplate.name);
        object.mesh.vertices = &CubeMesh::vertices();
        object.mesh.indices = &CubeMesh::indices();
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

        glm::dvec3 parentPositionKm = {0.0, 0.0, 0.0};
        if (body.parentBodyIndex.has_value()) {
            parentPositionKm = objects[body.parentBodyIndex.value()].transform.worldPosition / KM_TO_WORLD_UNITS;
        }

        glm::dvec3 orbitOffsetKm = {0.0, 0.0, 0.0};
        if (body.orbitRadiusKm > 0.0 && body.orbitPeriodDays > 0.0) {
            const double angle = glm::radians(body.orbitPhaseDegrees) + tau * (elapsedSimulationDays / body.orbitPeriodDays);
            orbitOffsetKm.x = std::cos(angle) * body.orbitRadiusKm;
            orbitOffsetKm.y = std::sin(angle) * body.orbitRadiusKm;
        }

        glm::dvec3 worldPositionKm = parentPositionKm + orbitOffsetKm;
        object.transform.worldPosition = worldPositionKm * KM_TO_WORLD_UNITS;

        if (body.rotationPeriodHours > 0.0) {
            const double rotationDegrees = wrapDegrees((elapsedSimulationDays * 24.0 / body.rotationPeriodHours) * 360.0);
            object.transform.rotationEulerDegrees = glm::vec3(0.0f, 0.0f, static_cast<float>(rotationDegrees));
        }
    }
}

double Scene::wrapDegrees(double angleDegrees) {
    double wrapped = std::fmod(angleDegrees, 360.0);
    if (wrapped < 0.0) {
        wrapped += 360.0;
    }
    return wrapped;
}
