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

void Player::updateFromInput(SDL_Window* window, float deltaTimeSeconds) {
    const bool movementInputPressed = isMovementInputPressed();

    const bool* keyState = SDL_GetKeyboardState(nullptr);

    const bool isEscPressed = keyState[SDL_SCANCODE_ESCAPE];
    if (isEscPressed && !escWasPressed) {
        mouseCaptured = !mouseCaptured;
        firstMouseSample = true;
    }
    escWasPressed = isEscPressed;

    float speedMultiplier = 1.0f;
    if (keyState[SDL_SCANCODE_LSHIFT]) {
        speedMultiplier = sprintMultiplier;
    }

    const float velocity = moveSpeedUnitsPerSecond * speedMultiplier * deltaTimeSeconds;

    if (orbitFollowActive) {
        orbitCancelGraceSeconds = std::max(0.0f, orbitCancelGraceSeconds - deltaTimeSeconds);
        if (orbitCancelGraceSeconds <= 0.0f && movementInputPressed && !movementInputWasPressedLastFrame) {
            cancelOrbitFollow();
        }
    }

    if (!orbitFollowActive) {
        if (keyState[SDL_SCANCODE_W]) {
            playerWorldPosition += glm::dvec3(front) * static_cast<double>(velocity);
        }
        if (keyState[SDL_SCANCODE_S]) {
            playerWorldPosition -= glm::dvec3(front) * static_cast<double>(velocity);
        }
        if (keyState[SDL_SCANCODE_A]) {
            playerWorldPosition -= glm::dvec3(right) * static_cast<double>(velocity);
        }
        if (keyState[SDL_SCANCODE_D]) {
            playerWorldPosition += glm::dvec3(right) * static_cast<double>(velocity);
        }
        if (keyState[SDL_SCANCODE_E]) {
            playerWorldPosition += glm::dvec3(worldUp) * static_cast<double>(velocity);
        }
        if (keyState[SDL_SCANCODE_Q]) {
            playerWorldPosition -= glm::dvec3(worldUp) * static_cast<double>(velocity);
        }
    }

    const SDL_WindowFlags windowFlags = SDL_GetWindowFlags(window);
    const bool isFocused = (windowFlags & SDL_WINDOW_INPUT_FOCUS) != 0;
    if (!isFocused) {
        SDL_SetWindowRelativeMouseMode(window, false);
        firstMouseSample = true;
        playerCamera.position = {0.0f, 0.0f, 0.0f};
        playerCamera.target = front;
        playerCamera.worldPosition = playerWorldPosition;
        movementInputWasPressedLastFrame = movementInputPressed;
        return;
    }

    if (!mouseCaptured) {
        SDL_SetWindowRelativeMouseMode(window, false);
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
        movementInputWasPressedLastFrame = movementInputPressed;
        return;
    }

    SDL_SetWindowRelativeMouseMode(window, true);

    float relX = 0.0f, relY = 0.0f;
    SDL_GetRelativeMouseState(&relX, &relY);

    if (firstMouseSample) {
        relX = 0.0f;
        relY = 0.0f;
        firstMouseSample = false;
    }

    const float offsetX = relX * lookSensitivity;
    const float offsetY = -relY * lookSensitivity;

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
    movementInputWasPressedLastFrame = movementInputPressed;
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
    orbitCancelGraceSeconds = 0.2f;
    movementInputWasPressedLastFrame = false;
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
    playerCamera.target = front;
}

bool Player::isOrbitFollowActive() const {
    return orbitFollowActive;
}

bool Player::isMovementInputPressed() const {
    const bool* keyState = SDL_GetKeyboardState(nullptr);
    return keyState[SDL_SCANCODE_W] ||
           keyState[SDL_SCANCODE_A] ||
           keyState[SDL_SCANCODE_S] ||
           keyState[SDL_SCANCODE_D] ||
           keyState[SDL_SCANCODE_Q] ||
           keyState[SDL_SCANCODE_E];
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
