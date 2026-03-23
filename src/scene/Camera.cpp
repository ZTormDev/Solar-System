#include "Camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

glm::mat4 Camera::viewMatrix() const {
    return glm::lookAt(position, target, up);
}

glm::mat4 Camera::projectionMatrix(float aspectRatio) const {
    glm::mat4 projection = glm::perspective(
        glm::radians(fieldOfViewDegrees),
        aspectRatio,
        nearPlane,
        farPlane
    );
    projection[1][1] *= -1.0f;
    return projection;
}
