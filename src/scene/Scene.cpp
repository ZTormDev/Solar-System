#include "Scene.hpp"

#include "scene/SphereMesh.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>

#include <glm/gtc/constants.hpp>

namespace {
constexpr std::array<std::pair<uint32_t, uint32_t>, 5> kSphereLods = {
    std::pair<uint32_t, uint32_t>{128u, 256u},
    std::pair<uint32_t, uint32_t>{96u, 192u},
    std::pair<uint32_t, uint32_t>{72u, 144u},
    std::pair<uint32_t, uint32_t>{48u, 96u},
    std::pair<uint32_t, uint32_t>{32u, 64u}
};

constexpr std::array<double, 4> kLodRatioThresholds = {5.0, 10.0, 22.0, 55.0};
constexpr double kLodHysteresisFactor = 0.18;

uint32_t chooseLodWithHysteresis(double distanceToRadiusRatio, uint32_t currentLod, uint32_t maxLod) {
    if (maxLod == 0) {
        return 0;
    }

    currentLod = std::min(currentLod, maxLod);

    while (currentLod > 0) {
        const std::size_t boundaryIndex = static_cast<std::size_t>(currentLod - 1);
        if (boundaryIndex >= kLodRatioThresholds.size()) {
            break;
        }

        const double boundary = kLodRatioThresholds[boundaryIndex] * (1.0 - kLodHysteresisFactor);
        if (distanceToRadiusRatio < boundary) {
            --currentLod;
            continue;
        }
        break;
    }

    while (currentLod < maxLod) {
        const std::size_t boundaryIndex = static_cast<std::size_t>(currentLod);
        if (boundaryIndex >= kLodRatioThresholds.size()) {
            break;
        }

        const double boundary = kLodRatioThresholds[boundaryIndex] * (1.0 + kLodHysteresisFactor);
        if (distanceToRadiusRatio > boundary) {
            ++currentLod;
            continue;
        }
        break;
    }

    return currentLod;
}
}

Scene::Scene() {
    buildLodSphereMeshes();
    initializeSolarMvpBodies();
    initializePhysicsState();
    syncObjectsFromPhysics();
    updateBodyRotations();
    updateValidationReport();
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

void Scene::update(SDL_Window* window, float timeSeconds, float deltaTimeSeconds) {
    elapsedSeconds = timeSeconds;

    if (window != nullptr) {
        player.updateFromInput(window, deltaTimeSeconds);
    }

    if (!paused) {
        const double simulatedSeconds = static_cast<double>(deltaTimeSeconds) * static_cast<double>(simulationTimeScale);
        const uint32_t substepCount = static_cast<uint32_t>(std::max(1.0, std::ceil(simulatedSeconds / MAX_PHYSICS_SUBSTEP_SECONDS)));
        const double substepSeconds = simulatedSeconds / static_cast<double>(substepCount);
        for (uint32_t substepIndex = 0; substepIndex < substepCount; ++substepIndex) {
            stepNBody(substepSeconds);
        }
        elapsedSimulationDays += simulatedSeconds / REAL_SECONDS_PER_DAY;
    }

    syncObjectsFromPhysics();
    updateBodyRotations();
    updateValidationReport();
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

double Scene::currentJulianDay() const {
    return J2000_JULIAN_DAY + elapsedSimulationDays;
}

const std::vector<CelestialBody>& Scene::celestialBodies() const {
    return bodies;
}

const std::vector<Scene::EphemerisValidationEntry>& Scene::ephemerisValidationEntries() const {
    return validationEntries;
}

glm::dvec3 Scene::sunWorldPosition() const {
    const std::size_t count = std::min(bodies.size(), objects.size());
    for (std::size_t index = 0; index < count; ++index) {
        if (bodies[index].name == "Sun") {
            return objects[index].transform.worldPosition;
        }
    }

    return glm::dvec3(0.0);
}

void Scene::followBody(std::size_t bodyIndex) {
    if (bodyIndex >= objects.size()) {
        return;
    }

    currentFollowedBodyIndex = bodyIndex;
    const GameObject& object = objects[bodyIndex];
    const double bodyRadiusUnits = std::max(1.0, static_cast<double>(object.transform.scale.x));
    const double desiredDistanceFromSurface = std::max(10000.0, bodyRadiusUnits * 2.5);
    const double targetDistanceFromCenter = bodyRadiusUnits + desiredDistanceFromSurface;
    player.beginOrbitFollow(object.transform.worldPosition, targetDistanceFromCenter);
}

void Scene::clearFollowBody() {
    currentFollowedBodyIndex.reset();
    player.cancelOrbitFollow();
}

std::optional<std::size_t> Scene::followedBodyIndex() const {
    return currentFollowedBodyIndex;
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
        .axialTiltDeg = 7.25,
        .orbitalElementsJ2000 = std::nullopt,
        .referenceCheckpoints = {},
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
        .axialTiltDeg = 23.439281,
        .orbitalElementsJ2000 = OrbitalElementsJ2000{
            .semiMajorAxisKm = 149598023.0,
            .eccentricity = 0.0167086,
            .inclinationDeg = 0.00005,
            .longitudeAscendingNodeDeg = -11.26064,
            .argumentPeriapsisDeg = 114.20783,
            .meanAnomalyDegAtJ2000 = 357.51716,
            .meanMotionDegPerDay = 0.9856076686
        },
        .referenceCheckpoints = {},
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
        .axialTiltDeg = 6.68,
        .orbitalElementsJ2000 = OrbitalElementsJ2000{
            .semiMajorAxisKm = 384399.0,
            .eccentricity = 0.0549,
            .inclinationDeg = 5.145,
            .longitudeAscendingNodeDeg = 125.08,
            .argumentPeriapsisDeg = 318.15,
            .meanAnomalyDegAtJ2000 = 115.3654,
            .meanMotionDegPerDay = 13.176358
        },
        .referenceCheckpoints = {},
        .parentBodyIndex = 1
    });
}

void Scene::initializePhysicsState() {
    bodyHeliocentricPositionsKm.assign(bodies.size(), glm::dvec3(0.0));
    bodyHeliocentricVelocitiesKmPerSec.assign(bodies.size(), glm::dvec3(0.0));

    std::function<std::pair<glm::dvec3, glm::dvec3>(std::size_t, double)> evaluateStateAtJulianDay =
        [this, &evaluateStateAtJulianDay](std::size_t bodyIndex, double julianDay) -> std::pair<glm::dvec3, glm::dvec3> {
        const CelestialBody& body = bodies[bodyIndex];

        glm::dvec3 positionKm(0.0);
        glm::dvec3 velocityKmPerSec(0.0);
        if (body.orbitalElementsJ2000.has_value()) {
            const auto [relativePositionKm, relativeVelocityKmPerSec] = orbitalStateFromElements(body.orbitalElementsJ2000.value(), julianDay);
            positionKm = relativePositionKm;
            velocityKmPerSec = relativeVelocityKmPerSec;
        }

        if (body.parentBodyIndex.has_value()) {
            const auto [parentPositionKm, parentVelocityKmPerSec] = evaluateStateAtJulianDay(body.parentBodyIndex.value(), julianDay);
            positionKm += parentPositionKm;
            velocityKmPerSec += parentVelocityKmPerSec;
        }

        return {positionKm, velocityKmPerSec};
    };

    const double initialJulianDay = currentJulianDay();
    for (std::size_t bodyIndex = 0; bodyIndex < bodies.size(); ++bodyIndex) {
        auto [positionKm, velocityKmPerSec] = evaluateStateAtJulianDay(bodyIndex, initialJulianDay);
        bodyHeliocentricPositionsKm[bodyIndex] = positionKm;
        bodyHeliocentricVelocitiesKmPerSec[bodyIndex] = velocityKmPerSec;
    }

    double totalMassKg = 0.0;
    glm::dvec3 weightedPositionSum(0.0);
    glm::dvec3 weightedVelocitySum(0.0);
    for (std::size_t bodyIndex = 0; bodyIndex < bodies.size(); ++bodyIndex) {
        const double massKg = bodies[bodyIndex].massKg;
        totalMassKg += massKg;
        weightedPositionSum += bodyHeliocentricPositionsKm[bodyIndex] * massKg;
        weightedVelocitySum += bodyHeliocentricVelocitiesKmPerSec[bodyIndex] * massKg;
    }

    if (totalMassKg > 0.0) {
        const glm::dvec3 barycenterPositionKm = weightedPositionSum / totalMassKg;
        const glm::dvec3 barycenterVelocityKmPerSec = weightedVelocitySum / totalMassKg;
        for (std::size_t bodyIndex = 0; bodyIndex < bodies.size(); ++bodyIndex) {
            bodyHeliocentricPositionsKm[bodyIndex] -= barycenterPositionKm;
            bodyHeliocentricVelocitiesKmPerSec[bodyIndex] -= barycenterVelocityKmPerSec;
        }
    }

    constexpr double kReferenceJulianDay2025 = 2460676.5;
    for (std::size_t bodyIndex = 0; bodyIndex < bodies.size(); ++bodyIndex) {
        CelestialBody& body = bodies[bodyIndex];
        body.referenceCheckpoints.clear();

        const auto [referenceJ2000PositionKm, _referenceJ2000Velocity] = evaluateStateAtJulianDay(bodyIndex, J2000_JULIAN_DAY);
        body.referenceCheckpoints.push_back(EphemerisReferenceCheckpoint{
            .julianDay = J2000_JULIAN_DAY,
            .heliocentricPositionKm = referenceJ2000PositionKm
        });

        const auto [reference2025PositionKm, _reference2025Velocity] = evaluateStateAtJulianDay(bodyIndex, kReferenceJulianDay2025);
        body.referenceCheckpoints.push_back(EphemerisReferenceCheckpoint{
            .julianDay = kReferenceJulianDay2025,
            .heliocentricPositionKm = reference2025PositionKm
        });
    }
}

std::vector<glm::dvec3> Scene::computeAccelerations(const std::vector<glm::dvec3>& positionsKm) const {
    std::vector<glm::dvec3> accelerations(positionsKm.size(), glm::dvec3(0.0));

    for (std::size_t bodyIndex = 0; bodyIndex < positionsKm.size(); ++bodyIndex) {
        for (std::size_t otherIndex = 0; otherIndex < positionsKm.size(); ++otherIndex) {
            if (bodyIndex == otherIndex) {
                continue;
            }

            const glm::dvec3 deltaKm = positionsKm[otherIndex] - positionsKm[bodyIndex];
            const double distanceSquaredKm = glm::dot(deltaKm, deltaKm);
            if (distanceSquaredKm < 1.0) {
                continue;
            }

            const double distanceKm = std::sqrt(distanceSquaredKm);
            const glm::dvec3 direction = deltaKm / distanceKm;
            const double accelerationMagnitudeKmPerSec2 = G_KM3_PER_KG_S2 * bodies[otherIndex].massKg / distanceSquaredKm;
            accelerations[bodyIndex] += direction * accelerationMagnitudeKmPerSec2;
        }
    }

    return accelerations;
}

void Scene::stepNBody(double deltaTimeSeconds) {
    if (bodyHeliocentricPositionsKm.empty() || bodyHeliocentricVelocitiesKmPerSec.empty()) {
        return;
    }

    const std::vector<glm::dvec3> a0 = computeAccelerations(bodyHeliocentricPositionsKm);
    std::vector<glm::dvec3> nextPositions = bodyHeliocentricPositionsKm;
    for (std::size_t bodyIndex = 0; bodyIndex < bodies.size(); ++bodyIndex) {
        nextPositions[bodyIndex] += bodyHeliocentricVelocitiesKmPerSec[bodyIndex] * deltaTimeSeconds +
                                    0.5 * a0[bodyIndex] * deltaTimeSeconds * deltaTimeSeconds;
    }

    const std::vector<glm::dvec3> a1 = computeAccelerations(nextPositions);
    for (std::size_t bodyIndex = 0; bodyIndex < bodies.size(); ++bodyIndex) {
        bodyHeliocentricVelocitiesKmPerSec[bodyIndex] += 0.5 * (a0[bodyIndex] + a1[bodyIndex]) * deltaTimeSeconds;
    }

    bodyHeliocentricPositionsKm = std::move(nextPositions);
}

std::pair<glm::dvec3, glm::dvec3> Scene::orbitalStateFromElements(const OrbitalElementsJ2000& elements, double julianDay) const {
    const double daysSinceJ2000 = julianDay - J2000_JULIAN_DAY;
    const double meanAnomalyRad = glm::radians(elements.meanAnomalyDegAtJ2000 + elements.meanMotionDegPerDay * daysSinceJ2000);
    const double eccentricAnomalyRad = solveEccentricAnomalyRad(meanAnomalyRad, elements.eccentricity);

    const double cosE = std::cos(eccentricAnomalyRad);
    const double sinE = std::sin(eccentricAnomalyRad);
    const double sqrtOneMinusESquared = std::sqrt(1.0 - elements.eccentricity * elements.eccentricity);

    const double orbitalX = elements.semiMajorAxisKm * (cosE - elements.eccentricity);
    const double orbitalY = elements.semiMajorAxisKm * sqrtOneMinusESquared * sinE;

    const double meanMotionRadPerSec = glm::radians(elements.meanMotionDegPerDay) / REAL_SECONDS_PER_DAY;
    const double dEdt = meanMotionRadPerSec / std::max(1e-12, (1.0 - elements.eccentricity * cosE));
    const double orbitalVx = -elements.semiMajorAxisKm * sinE * dEdt;
    const double orbitalVy = elements.semiMajorAxisKm * sqrtOneMinusESquared * cosE * dEdt;

    const double cosOmega = std::cos(glm::radians(elements.longitudeAscendingNodeDeg));
    const double sinOmega = std::sin(glm::radians(elements.longitudeAscendingNodeDeg));
    const double cosI = std::cos(glm::radians(elements.inclinationDeg));
    const double sinI = std::sin(glm::radians(elements.inclinationDeg));
    const double cosW = std::cos(glm::radians(elements.argumentPeriapsisDeg));
    const double sinW = std::sin(glm::radians(elements.argumentPeriapsisDeg));

    const auto rotatePerifocalToInertial = [cosOmega, sinOmega, cosI, sinI, cosW, sinW](double x, double y) {
        const double xKm =
            (cosOmega * cosW - sinOmega * sinW * cosI) * x +
            (-cosOmega * sinW - sinOmega * cosW * cosI) * y;
        const double yKm =
            (sinOmega * cosW + cosOmega * sinW * cosI) * x +
            (-sinOmega * sinW + cosOmega * cosW * cosI) * y;
        const double zKm =
            (sinW * sinI) * x +
            (cosW * sinI) * y;
        return glm::dvec3(xKm, yKm, zKm);
    };

    return {
        rotatePerifocalToInertial(orbitalX, orbitalY),
        rotatePerifocalToInertial(orbitalVx, orbitalVy)
    };
}

glm::dvec3 Scene::heliocentricPositionFromElements(const OrbitalElementsJ2000& elements, double julianDay) const {
    return orbitalStateFromElements(elements, julianDay).first;
}

double Scene::solveEccentricAnomalyRad(double meanAnomalyRad, double eccentricity) {
    double normalizedMeanAnomaly = wrapRadians(meanAnomalyRad);
    double eccentricAnomaly = normalizedMeanAnomaly;

    for (int iteration = 0; iteration < 10; ++iteration) {
        const double f = eccentricAnomaly - eccentricity * std::sin(eccentricAnomaly) - normalizedMeanAnomaly;
        const double fPrime = 1.0 - eccentricity * std::cos(eccentricAnomaly);
        if (std::abs(fPrime) < 1e-12) {
            break;
        }

        const double delta = f / fPrime;
        eccentricAnomaly -= delta;
        if (std::abs(delta) < 1e-12) {
            break;
        }
    }

    return eccentricAnomaly;
}

void Scene::updateValidationReport() {
    validationEntries.clear();

    if (bodyHeliocentricPositionsKm.size() != bodies.size()) {
        return;
    }

    const double julianDay = currentJulianDay();
    std::function<glm::dvec3(std::size_t)> evaluateAnalyticHeliocentric =
        [this, julianDay, &evaluateAnalyticHeliocentric](std::size_t bodyIndex) -> glm::dvec3 {
        const CelestialBody& body = bodies[bodyIndex];
        glm::dvec3 analyticPositionKm(0.0);
        if (body.orbitalElementsJ2000.has_value()) {
            analyticPositionKm = heliocentricPositionFromElements(body.orbitalElementsJ2000.value(), julianDay);
        }

        if (body.parentBodyIndex.has_value()) {
            analyticPositionKm += evaluateAnalyticHeliocentric(body.parentBodyIndex.value());
        }

        return analyticPositionKm;
    };

    for (std::size_t bodyIndex = 0; bodyIndex < bodies.size(); ++bodyIndex) {
        const CelestialBody& body = bodies[bodyIndex];
        const glm::dvec3 analyticPositionKm = evaluateAnalyticHeliocentric(bodyIndex);
        const double errorKm = glm::length(bodyHeliocentricPositionsKm[bodyIndex] - analyticPositionKm);
        validationEntries.push_back(EphemerisValidationEntry{
            .bodyName = body.name,
            .referenceJulianDay = julianDay,
            .errorKm = errorKm
        });
    }
}

void Scene::syncObjectsFromPhysics() {
    if (bodyHeliocentricPositionsKm.empty()) {
        return;
    }
    const uint32_t maxLod = lodMeshSlices.empty() ? 0u : static_cast<uint32_t>(lodMeshSlices.size() - 1);

    for (std::size_t index = 0; index < bodies.size(); ++index) {
        GameObject& object = objects[index];
        object.transform.worldPosition = bodyHeliocentricPositionsKm[index] * METERS_PER_KILOMETER;

        const double distanceToCamera = glm::length(object.transform.worldPosition - player.worldPosition());
        const double bodyRadiusUnits = std::max(1.0, static_cast<double>(object.transform.scale.x));
        const double distanceToRadiusRatio = distanceToCamera / bodyRadiusUnits;
        object.meshSliceIndex = chooseLodWithHysteresis(distanceToRadiusRatio, object.meshSliceIndex, maxLod);
    }

    if (currentFollowedBodyIndex.has_value()) {
        const std::size_t followedIndex = currentFollowedBodyIndex.value();
        if (followedIndex < objects.size() && player.isOrbitFollowActive()) {
            player.updateOrbitFollowTarget(objects[followedIndex].transform.worldPosition);
        } else {
            currentFollowedBodyIndex.reset();
            player.cancelOrbitFollow();
        }
    } else if (!player.isOrbitFollowActive()) {
        currentFollowedBodyIndex.reset();
    }
}

void Scene::updateBodyRotations() {
    for (std::size_t index = 0; index < bodies.size(); ++index) {
        const CelestialBody& body = bodies[index];
        GameObject& object = objects[index];

        if (body.rotationPeriodHours > 0.0) {
            const double rotationDegrees = wrapDegrees((elapsedSimulationDays * 24.0 / body.rotationPeriodHours) * 360.0);
            object.transform.rotationEulerDegrees = glm::vec3(
                static_cast<float>(body.axialTiltDeg),
                0.0f,
                static_cast<float>(rotationDegrees)
            );
        } else {
            object.transform.rotationEulerDegrees = glm::vec3(static_cast<float>(body.axialTiltDeg), 0.0f, 0.0f);
        }
    }
}

uint32_t Scene::lodForDistance(double distanceToCameraUnits, double bodyRadiusUnits) const {
    const double distanceToRadiusRatio = distanceToCameraUnits / std::max(bodyRadiusUnits, 1.0);

    if (distanceToRadiusRatio < kLodRatioThresholds[0]) {
        return 0;
    }
    if (distanceToRadiusRatio < kLodRatioThresholds[1]) {
        return 1;
    }
    if (distanceToRadiusRatio < kLodRatioThresholds[2]) {
        return 2;
    }
    if (distanceToRadiusRatio < kLodRatioThresholds[3]) {
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

double Scene::wrapRadians(double angleRadians) {
    constexpr double twoPi = glm::two_pi<double>();
    double wrapped = std::fmod(angleRadians, twoPi);
    if (wrapped < 0.0) {
        wrapped += twoPi;
    }
    return wrapped;
}
