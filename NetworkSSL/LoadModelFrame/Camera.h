#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

enum class CameraType {
    ORBIT,      // 轨道相机
    FPS,        // 第一人称相机
    FREE        // 自由相机
};

class Camera {
public:
    // 轨道相机参数
    glm::vec3 target = glm::vec3(0.0f);
    float radius = 5.0f;
    float yaw = 45.0f;
    float pitch = 45.0f;

    // FPS/自由相机参数
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    // 相机属性
    float fov = 45.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

    // 控制参数
    float mouseSensitivity = 100.0f;
    float scrollSensitivity = 2.0f;
    float moveSpeed = 2.5f;

    CameraType type = CameraType::ORBIT;

public:
    Camera(CameraType camType = CameraType::ORBIT);

    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetBackViewMatrix() const;
    glm::mat4 GetProjectionMatrix(float aspectRatio) const;

    void ProcessMouseMovement(float deltaX, float deltaY);
    void ProcessMouseScroll(float deltaY);
    void ProcessKeyboard(int direction, float deltaTime); // direction: WASD对应0123

    void SetTarget(const glm::vec3& newTarget);
    void SetType(CameraType newType);
    CameraType GetType() const { return type; }

private:
    void updateCameraVectors();
    void updateOrbitPosition();
};