#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Camera {
public:
    glm::vec3 position{0.0f, 2.2f, 0.0f};
    glm::quat orientation{1.0f, 0.0f, 0.0f, 0.0f};

    float moveSpeed = 5.0f;
    float mouseSensitivity = 0.00085f;
    float fov = 55.0f;
    float nearPlane = 0.1f;
    float farPlane = 120.0f;
    float pitchLimit = 1.55334f;

    void setLook(float yawRad, float pitchRad);
    glm::quat orientationQuat() const;
    glm::mat4 viewMatrix() const;
    glm::mat4 projectionMatrix(float aspect) const;

    glm::vec3 forward() const;
    glm::vec3 right() const;
    glm::vec3 up() const;

    void processMouse(float dx, float dy);
    void processKeyboard(bool forwardKey, bool backKey, bool leftKey, bool rightKey, bool upKey, bool downKey, float dt);
};
