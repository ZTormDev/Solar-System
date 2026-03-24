#pragma once

#include "scene/Camera.hpp"

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

class Player {
public:
    Player();

    void updateFromInput(GLFWwindow* window, float deltaTimeSeconds);
    void adjustMoveSpeedFromMouseWheel(float mouseWheelDelta, bool uiCapturingMouse);
    float moveSpeedUnitsPerSecondValue() const;
    void setSprintMultiplier(float value);
    float sprintMultiplierValue() const;

    Camera& camera();
    const Camera& camera() const;
    const glm::dvec3& worldPosition() const;
    void teleportToWorldPosition(const glm::dvec3& worldPosition);
    void beginOrbitFollow(const glm::dvec3& targetWorldPosition, double desiredDistanceUnits);
    void updateOrbitFollowTarget(const glm::dvec3& targetWorldPosition);
    void cancelOrbitFollow();
    bool isOrbitFollowActive() const;

private:
    bool isMovementInputPressed(GLFWwindow* window) const;
    void updateCameraVectors();

    Camera playerCamera;

    glm::dvec3 playerWorldPosition = {147105290000.0, 0.0, 0.0};

    glm::vec3 front = {0.0f, -1.0f, 0.0f};
    glm::vec3 right = {1.0f, 0.0f, 0.0f};
    glm::vec3 worldUp = {0.0f, 0.0f, 1.0f};

    float yawDegrees = -90.0f;
    float pitchDegrees = 0.0f;

    float moveSpeedUnitsPerSecond = 100000.0f;
    float sprintMultiplier = 3.0f;
    float lookSensitivity = 0.12f;

    bool orbitFollowActive = false;
    glm::dvec3 orbitTargetWorldPosition = {0.0, 0.0, 0.0};
    double orbitDistanceUnits = 1000000.0;
    float orbitCancelGraceSeconds = 0.0f;
    bool movementInputWasPressedLastFrame = false;

    bool mouseCaptured = true;
    bool escWasPressed = false;

    bool firstMouseSample = true;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
};
