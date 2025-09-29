#include "Camera.h"
#include <algorithm>

Camera::Camera(CameraType camType) : type(camType) {
    updateCameraVectors();
    if (type == CameraType::ORBIT) {
        updateOrbitPosition();
    }
}

glm::mat4 Camera::GetViewMatrix() const {
    if (type == CameraType::ORBIT) {
        return glm::lookAt(position, target, up);
    }
    else {
        return glm::lookAt(position, position + front, up);
    }
}

glm::mat4 Camera::GetProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}

void Camera::ProcessMouseMovement(float deltaX, float deltaY) {
    deltaX *= mouseSensitivity;
    deltaY *= mouseSensitivity;

    if (type == CameraType::ORBIT) {
        yaw += deltaX * 0.01f;
        pitch -= deltaY * 0.01f;

        // 限制pitch角度
        pitch = std::clamp(pitch, -89.0f, 89.0f);

        updateOrbitPosition();
    }
    else {
        yaw += deltaX * 0.01f;
        pitch += deltaY * 0.01f;

        // 限制pitch角度
        pitch = std::clamp(pitch, -89.0f, 89.0f);

        updateCameraVectors();
    }
}

void Camera::ProcessMouseScroll(float deltaY) {
    if (type == CameraType::ORBIT) {
        radius -= deltaY * scrollSensitivity;
        radius = std::clamp(radius, 1.0f, 50.0f);
        updateOrbitPosition();
    }
    else {
        fov -= deltaY;
        fov = std::clamp(fov, 1.0f, 90.0f);
    }
}

void Camera::ProcessKeyboard(int direction, float deltaTime) {
    float velocity = moveSpeed * deltaTime;

    if (type == CameraType::ORBIT) {
        // 轨道相机移动目标点
        switch (direction) {
        case 0: target += front * velocity; break;  // W
        case 1: target -= right * velocity; break;  // A
        case 2: target -= front * velocity; break;  // S
        case 3: target += right * velocity; break;  // D
        }
        updateOrbitPosition();
    }
    else {
        // FPS/自由相机移动相机位置
        switch (direction) {
        case 0: position += front * velocity; break;  // W
        case 1: position -= right * velocity; break;  // A
        case 2: position -= front * velocity; break;  // S
        case 3: position += right * velocity; break;  // D
        }
    }
}

void Camera::SetTarget(const glm::vec3& newTarget) {
    target = newTarget;
    if (type == CameraType::ORBIT) {
        updateOrbitPosition();
    }
}

void Camera::SetType(CameraType newType) {
    type = newType;
    if (type == CameraType::ORBIT) {
        updateOrbitPosition();
    }
    else {
        updateCameraVectors();
    }
}

void Camera::updateCameraVectors() {
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);

    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

void Camera::updateOrbitPosition() {
    float x = radius * cos(glm::radians(pitch)) * cos(glm::radians(yaw));
    float y = radius * sin(glm::radians(pitch));
    float z = radius * cos(glm::radians(pitch)) * sin(glm::radians(yaw));

    position = target + glm::vec3(x, y, z);
    front = glm::normalize(target - position);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}