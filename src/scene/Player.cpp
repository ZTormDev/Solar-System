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
    const bool movementInputPressed = isMovementInputPressed(window);

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

    if (orbitFollowActive) {
        orbitCancelGraceSeconds = std::max(0.0f, orbitCancelGraceSeconds - deltaTimeSeconds);
        if (orbitCancelGraceSeconds <= 0.0f) {
            if (!orbitCancelMovementArmed) {
                if (!movementInputPressed) {
                    orbitCancelMovementArmed = true;
                }
            } else if (movementInputPressed) {
                cancelOrbitFollow();
            }
        }
    }

    if (!orbitFollowActive) {
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

    if (!mouseCaptured) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstMouseSample = true;
        if (orbitFollowActive) {
            const glm::dvec3 targetDirection = orbitTargetWorldPosition - playerWorldPosition;
            if (glm::length(targetDirection) > 0.0) {
                playerCamera.target = glm::normalize(glm::vec3(targetDirection));
            } else {
                playerCamera.target = front;
            }
        } else {
            playerCamera.target = front;
        }
        playerCamera.position = {0.0f, 0.0f, 0.0f};
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

    if (orbitFollowActive) {
        playerWorldPosition = orbitTargetWorldPosition - glm::dvec3(front) * orbitDistanceUnits;
        const glm::dvec3 targetDirection = orbitTargetWorldPosition - playerWorldPosition;
        if (glm::length(targetDirection) > 0.0) {
            playerCamera.target = glm::normalize(glm::vec3(targetDirection));
        } else {
            playerCamera.target = front;
        }
    } else {
        playerCamera.target = front;
    }

    playerCamera.position = {0.0f, 0.0f, 0.0f};
    playerCamera.worldPosition = playerWorldPosition;
}

void Player::adjustMoveSpeedFromMouseWheel(float mouseWheelDelta, bool uiCapturingMouse) {
    if (uiCapturingMouse || mouseWheelDelta == 0.0f) {
        return;
    }

    if (orbitFollowActive) {
        constexpr double minOrbitDistance = 1000.0;
        constexpr double zoomExponentialBasePerWheelTick = 1.2;
        orbitDistanceUnits = std::max(
            minOrbitDistance,
            orbitDistanceUnits / std::pow(zoomExponentialBasePerWheelTick, static_cast<double>(mouseWheelDelta))
        );

        playerWorldPosition = orbitTargetWorldPosition - glm::dvec3(front) * orbitDistanceUnits;
        playerCamera.worldPosition = playerWorldPosition;
        return;
    }

    constexpr float minMoveSpeed = 0.25f;
    constexpr float speedExponentialBasePerWheelTick = 1.2f;

    // no max move speed
    moveSpeedUnitsPerSecond = std::max(
        minMoveSpeed,
        moveSpeedUnitsPerSecond * std::pow(speedExponentialBasePerWheelTick, mouseWheelDelta)
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

void Player::teleportToWorldPosition(const glm::dvec3& worldPosition) {
    playerWorldPosition = worldPosition;
    playerCamera.worldPosition = playerWorldPosition;
}

void Player::beginOrbitFollow(const glm::dvec3& targetWorldPosition, double desiredDistanceUnits) {
    orbitFollowActive = true;
    mouseCaptured = true;
    firstMouseSample = true;
    orbitCancelGraceSeconds = 0.12f;
    orbitCancelMovementArmed = false;
    orbitTargetWorldPosition = targetWorldPosition;
    orbitDistanceUnits = std::max(1000.0, desiredDistanceUnits);

    glm::dvec3 awayFromTarget = playerWorldPosition - orbitTargetWorldPosition;
    const double awayLength = glm::length(awayFromTarget);
    if (awayLength <= 1e-9) {
        awayFromTarget = glm::dvec3(1.0, 0.0, 0.0);
    } else {
        awayFromTarget /= awayLength;
    }

    playerWorldPosition = orbitTargetWorldPosition + awayFromTarget * orbitDistanceUnits;

    const glm::dvec3 viewDirection = glm::normalize(orbitTargetWorldPosition - playerWorldPosition);
    yawDegrees = glm::degrees(std::atan2(static_cast<float>(viewDirection.y), static_cast<float>(viewDirection.x)));
    pitchDegrees = glm::degrees(std::asin(static_cast<float>(viewDirection.z)));
    pitchDegrees = std::clamp(pitchDegrees, -89.0f, 89.0f);

    updateCameraVectors();
    playerCamera.target = front;
    playerCamera.worldPosition = playerWorldPosition;
}

void Player::updateOrbitFollowTarget(const glm::dvec3& targetWorldPosition) {
    if (!orbitFollowActive) {
        return;
    }

    orbitTargetWorldPosition = targetWorldPosition;
    playerWorldPosition = orbitTargetWorldPosition - glm::dvec3(front) * orbitDistanceUnits;

    const glm::dvec3 targetDirection = orbitTargetWorldPosition - playerWorldPosition;
    if (glm::length(targetDirection) > 0.0) {
        playerCamera.target = glm::normalize(glm::vec3(targetDirection));
    } else {
        playerCamera.target = front;
    }
    playerCamera.worldPosition = playerWorldPosition;
}

void Player::cancelOrbitFollow() {
    orbitFollowActive = false;
    orbitCancelGraceSeconds = 0.0f;
    orbitCancelMovementArmed = false;
    playerCamera.target = front;
}

bool Player::isOrbitFollowActive() const {
    return orbitFollowActive;
}

bool Player::isMovementInputPressed(GLFWwindow* window) const {
    return glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
           glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
           glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
           glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ||
           glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS ||
           glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS;
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
