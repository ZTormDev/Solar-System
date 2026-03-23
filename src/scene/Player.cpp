#include "Player.hpp"

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>

Player::Player() {
    playerCamera.position = {2.2f, 2.2f, 2.2f};
    playerCamera.up = worldUp;
    updateCameraVectors();
}

void Player::updateFromInput(GLFWwindow* window, float deltaTimeSeconds) {
    const bool isEscPressed = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (isEscPressed && !escWasPressed) {
        mouseCaptured = !mouseCaptured;
        firstMouseSample = true;
    }
    escWasPressed = isEscPressed;

    float speedMultiplier = 1.0f;
    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
        speedMultiplier = 2.5f;
    }

    const float velocity = moveSpeedUnitsPerSecond * speedMultiplier * deltaTimeSeconds;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        playerCamera.position += front * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        playerCamera.position -= front * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        playerCamera.position -= right * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        playerCamera.position += right * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        playerCamera.position += worldUp * velocity;
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        playerCamera.position -= worldUp * velocity;
    }

    const bool isFocused = glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE;
    if (!isFocused) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstMouseSample = true;
        playerCamera.target = playerCamera.position + front;
        return;
    }

    if (!mouseCaptured) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstMouseSample = true;
        playerCamera.target = playerCamera.position + front;
        return;
    }

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    double mouseX = 0.0;
    double mouseY = 0.0;
    glfwGetCursorPos(window, &mouseX, &mouseY);

    if (firstMouseSample) {
        lastMouseX = mouseX;
        lastMouseY = mouseY;
        firstMouseSample = false;
    }

    const float offsetX = static_cast<float>(mouseX - lastMouseX) * lookSensitivity;
    const float offsetY = static_cast<float>(lastMouseY - mouseY) * lookSensitivity;
    lastMouseX = mouseX;
    lastMouseY = mouseY;

    yawDegrees -= offsetX;
    pitchDegrees += offsetY;
    pitchDegrees = std::clamp(pitchDegrees, -89.0f, 89.0f);

    updateCameraVectors();

    playerCamera.target = playerCamera.position + front;
}

Camera& Player::camera() {
    return playerCamera;
}

const Camera& Player::camera() const {
    return playerCamera;
}

void Player::updateCameraVectors() {
    const float yawRadians = glm::radians(yawDegrees);
    const float pitchRadians = glm::radians(pitchDegrees);

    glm::vec3 direction;
    direction.x = cos(yawRadians) * cos(pitchRadians);
    direction.y = sin(yawRadians) * cos(pitchRadians);
    direction.z = sin(pitchRadians);

    front = glm::normalize(direction);
    right = glm::normalize(glm::cross(front, worldUp));
    playerCamera.up = glm::normalize(glm::cross(right, front));
    playerCamera.target = playerCamera.position + front;
}
