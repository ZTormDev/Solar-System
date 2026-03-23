#pragma once

#include <glm/glm.hpp>

class Transform {
public:
    glm::dvec3 worldPosition = {0.0, 0.0, 0.0};
    glm::vec3 rotationEulerDegrees = {0.0f, 0.0f, 0.0f};
    glm::vec3 scale = {1.0f, 1.0f, 1.0f};

    glm::mat4 modelMatrix(const glm::dvec3& floatingOrigin) const;
};
