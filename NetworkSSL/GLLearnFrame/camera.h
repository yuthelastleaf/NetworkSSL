#pragma once

#include "demo.h"

class CameraDemo : public Demo {
private:
    unsigned int VBO, VAO;
    std::unique_ptr<Shader> shader;
    // 其他需要的变量
    unsigned int EBO;
    unsigned int texture1, texture2;

public:
    void Initialize() override;
    void Update(float deltaTime) override;
    void Render() override;
    void RenderImGui() override;
    void Cleanup() override;
    void ProcInput(struct GLFWwindow* window, float deltaTime) override;
    std::string GetName() const override;

private:
    void createBasicTextures();
    void applyTextureSettings(unsigned int textureID, int wrapMode, bool useNearest);

private:

    float model_angle_ = -55.0f;
    float zoffset_ = -3.0f;
    float fovy_ = 45.0f;


    bool auto_rotate_ = false;
    float rotate_speed_ = 1.0f;

    glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
    float delta_camera_speed_ = 0.05f;
    float camera_speed_ = 2.0f;
    float mouse_sens_ = 2.0f;
    float scroll_sensitivity_ = 2.0f;

    float yaw_ = -90.0f;	// yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right so we initially rotate a bit to the left.
    float pitch_ = 0.0f;
};

