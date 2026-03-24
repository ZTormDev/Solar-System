#pragma once

#include <glm/glm.hpp>

class Camera {
public:
    glm::dvec3 worldPosition = {2.2, 2.2, 2.2};

    glm::vec3 position = {0.0f, 0.0f, 0.0f};
    glm::vec3 target = {0.0f, -1.0f, 0.0f};
    glm::vec3 up = {0.0f, 0.0f, 1.0f};

    float fieldOfViewDegrees = 75.0f;
    float nearPlane = 10000.0f;
    float farPlane = 300000000000.0f;

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspectRatio) const;
};
