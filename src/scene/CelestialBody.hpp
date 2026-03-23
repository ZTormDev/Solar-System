#pragma once

#include <cstddef>
#include <optional>
#include <string>

struct CelestialBody {
    std::string name;

    double radiusKm = 0.0;
    double massKg = 0.0;
    double orbitRadiusKm = 0.0;
    double orbitPeriodDays = 0.0;
    double rotationPeriodHours = 0.0;
    double orbitPhaseDegrees = 0.0;

    std::optional<std::size_t> parentBodyIndex;
};
