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

private:
    void updateCameraVectors();

    Camera playerCamera;

    glm::dvec3 playerWorldPosition = {2.2, 2.2, 2.2};

    glm::vec3 front = {0.0f, -1.0f, 0.0f};
    glm::vec3 right = {1.0f, 0.0f, 0.0f};
    glm::vec3 worldUp = {0.0f, 0.0f, 1.0f};

    float yawDegrees = -90.0f;
    float pitchDegrees = 0.0f;

    float moveSpeedUnitsPerSecond = 2.7f;
    float sprintMultiplier = 2.5f;
    float lookSensitivity = 0.12f;

    bool mouseCaptured = true;
    bool escWasPressed = false;

    bool firstMouseSample = true;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
};
