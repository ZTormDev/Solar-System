#include "Player.hpp"

#include <imgui.h>

#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

Player::Player() {
    playerCamera.position = {0.0f, 0.0f, 0.0f};
    playerCamera.worldPosition = playerWorldPosition;
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
        speedMultiplier = sprintMultiplier;
    }

    const float velocity = moveSpeedUnitsPerSecond * speedMultiplier * deltaTimeSeconds;

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
        playerWorldPosition += glm::dvec3(front) * static_cast<double>(velocity);
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
        playerWorldPosition -= glm::dvec3(front) * static_cast<double>(velocity);
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
        playerWorldPosition -= glm::dvec3(right) * static_cast<double>(velocity);
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
        playerWorldPosition += glm::dvec3(right) * static_cast<double>(velocity);
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS) {
        playerWorldPosition += glm::dvec3(worldUp) * static_cast<double>(velocity);
    }
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS) {
        playerWorldPosition -= glm::dvec3(worldUp) * static_cast<double>(velocity);
    }

    const bool isFocused = glfwGetWindowAttrib(window, GLFW_FOCUSED) == GLFW_TRUE;
    if (!isFocused) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstMouseSample = true;
        playerCamera.position = {0.0f, 0.0f, 0.0f};
        playerCamera.target = front;
        playerCamera.worldPosition = playerWorldPosition;
        return;
    }

    const bool imguiWantsMouse = ImGui::GetCurrentContext() != nullptr && ImGui::GetIO().WantCaptureMouse;
    if (imguiWantsMouse) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstMouseSample = true;
        playerCamera.position = {0.0f, 0.0f, 0.0f};
        playerCamera.target = front;
        playerCamera.worldPosition = playerWorldPosition;
        return;
    }

    if (!mouseCaptured) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstMouseSample = true;
        playerCamera.position = {0.0f, 0.0f, 0.0f};
        playerCamera.target = front;
        playerCamera.worldPosition = playerWorldPosition;
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

    playerCamera.position = {0.0f, 0.0f, 0.0f};
    playerCamera.target = front;
    playerCamera.worldPosition = playerWorldPosition;
}

void Player::adjustMoveSpeedFromMouseWheel(float mouseWheelDelta, bool uiCapturingMouse) {
    if (uiCapturingMouse || mouseWheelDelta == 0.0f) {
        return;
    }

    constexpr float minMoveSpeed = 0.25f;
    constexpr float maxMoveSpeed = 80.0f;
    constexpr float speedExponentialBasePerWheelTick = 1.24f;

    moveSpeedUnitsPerSecond = std::clamp(
        moveSpeedUnitsPerSecond * std::pow(speedExponentialBasePerWheelTick, mouseWheelDelta),
        minMoveSpeed,
        maxMoveSpeed
    );
}

float Player::moveSpeedUnitsPerSecondValue() const {
    return moveSpeedUnitsPerSecond;
}

void Player::setSprintMultiplier(float value) {
    sprintMultiplier = std::clamp(value, 1.0f, 10.0f);
}

float Player::sprintMultiplierValue() const {
    return sprintMultiplier;
}

Camera& Player::camera() {
    return playerCamera;
}

const Camera& Player::camera() const {
    return playerCamera;
}

const glm::dvec3& Player::worldPosition() const {
    return playerWorldPosition;
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
    playerCamera.position = {0.0f, 0.0f, 0.0f};
    playerCamera.target = front;
    playerCamera.worldPosition = playerWorldPosition;
}
