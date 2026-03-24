#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <glm/glm.hpp>

struct OrbitalElementsJ2000 {
    double semiMajorAxisKm = 0.0;
    double eccentricity = 0.0;
    double inclinationDeg = 0.0;
    double longitudeAscendingNodeDeg = 0.0;
    double argumentPeriapsisDeg = 0.0;
    double meanAnomalyDegAtJ2000 = 0.0;
    double meanMotionDegPerDay = 0.0;
};

struct EphemerisReferenceCheckpoint {
    double julianDay = 2451545.0;
    glm::dvec3 heliocentricPositionKm = glm::dvec3(0.0);
};

struct CelestialBody {
    std::string name;

    double radiusKm = 0.0;
    double massKg = 0.0;
    double orbitRadiusKm = 0.0;
    double orbitPeriodDays = 0.0;
    double rotationPeriodHours = 0.0;
    double orbitPhaseDegrees = 0.0;
    double axialTiltDeg = 0.0;

    std::optional<OrbitalElementsJ2000> orbitalElementsJ2000;
    std::vector<EphemerisReferenceCheckpoint> referenceCheckpoints;

    std::optional<std::size_t> parentBodyIndex;
};
