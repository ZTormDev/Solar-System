#include "Transform.hpp"

#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Transform::modelMatrix(const glm::dvec3& floatingOrigin) const {
    glm::mat4 model = glm::mat4(1.0f);

    glm::dvec3 relativePosition = worldPosition - floatingOrigin;
    model = glm::translate(model, glm::vec3(relativePosition));
    model = glm::rotate(model, glm::radians(rotationEulerDegrees.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotationEulerDegrees.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(rotationEulerDegrees.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, scale);
    return model;
}
