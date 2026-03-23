#pragma once

#include <glm/glm.hpp>

class Camera {
public:
    glm::vec3 position = {2.2f, 2.2f, 2.2f};
    glm::vec3 target = {0.0f, 0.0f, 0.0f};
    glm::vec3 up = {0.0f, 0.0f, 1.0f};

    float fieldOfViewDegrees = 75.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;

    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspectRatio) const;
};
