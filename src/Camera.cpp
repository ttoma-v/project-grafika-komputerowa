#include "Camera.h"

#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cmath>

void Camera::setLook(float yawRad, float pitchRad) {
    const glm::quat qYaw = glm::angleAxis(yawRad, glm::vec3(0.0f, 1.0f, 0.0f));
    const glm::quat qPitch = glm::angleAxis(pitchRad, glm::vec3(1.0f, 0.0f, 0.0f));
    orientation = glm::normalize(qYaw * qPitch);
}

glm::quat Camera::orientationQuat() const {
    return orientation;
}

glm::vec3 Camera::forward() const {
    return glm::normalize(orientationQuat() * glm::vec3(0.0f, 0.0f, -1.0f));
}

glm::vec3 Camera::right() const {
    return glm::normalize(orientationQuat() * glm::vec3(1.0f, 0.0f, 0.0f));
}

glm::vec3 Camera::up() const {
    return glm::normalize(orientationQuat() * glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::viewMatrix() const {
    const glm::mat4 world = glm::translate(glm::mat4(1.0f), position) * glm::mat4_cast(orientationQuat());
    return glm::inverse(world);
}

glm::mat4 Camera::projectionMatrix(float aspect) const {
    return glm::perspective(glm::radians(fov), aspect, nearPlane, farPlane);
}

void Camera::processMouse(float dx, float dy) {
    const float yawDelta = -dx * mouseSensitivity;
    float pitchDelta = dy * mouseSensitivity;

    // World-space yaw (pre-multiply) keeps the horizon level: no accumulated roll.
    orientation = glm::normalize(glm::angleAxis(yawDelta, glm::vec3(0.0f, 1.0f, 0.0f)) * orientation);

    // Limit total pitch so the view never flips past vertical.
    const float currentPitch = std::asin(std::clamp(forward().y, -1.0f, 1.0f));
    const float newPitch = std::clamp(currentPitch + pitchDelta, -pitchLimit, pitchLimit);
    pitchDelta = newPitch - currentPitch;

    // Local-space pitch (post-multiply) around the camera's own right axis.
    orientation = glm::normalize(orientation * glm::angleAxis(pitchDelta, glm::vec3(1.0f, 0.0f, 0.0f)));
}

void Camera::processKeyboard(bool fwd, bool back, bool left, bool rightKey, bool upKey, bool downKey, float dt) {
    const float speed = moveSpeed * dt;
    glm::vec3 f = forward();
    glm::vec3 r = right();
    f.y = 0.0f;
    r.y = 0.0f;
    if (glm::length(f) > 1e-5f) f = glm::normalize(f);
    if (glm::length(r) > 1e-5f) r = glm::normalize(r);

    if (fwd) position += f * speed;
    if (back) position -= f * speed;
    if (left) position -= r * speed;
    if (rightKey) position += r * speed;

    if (upKey) position.y += speed;
    if (downKey) position.y -= speed;

    position.y = std::clamp(position.y, 0.5f, 25.0f);
}
