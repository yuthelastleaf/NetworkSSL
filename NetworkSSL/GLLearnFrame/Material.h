#pragma once

#include "demo.h"

class MaterialDemo : public Demo {
private:
    unsigned int VBO, VAO;
    unsigned int lightVBO, lightVAO;
    std::unique_ptr<Shader> shader;
    std::unique_ptr<Shader> texture_shader;
    std::unique_ptr<Shader> light_shader;
    std::unique_ptr<Shader> glight_shader;
    // 其他需要的变量
    unsigned int EBO;
    unsigned int texture1, texture2;
    unsigned int whiteTexture;
    unsigned int redTexture;
    unsigned int greenTexture;
    unsigned int blueTexture;

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
    void updateOrbitCamera();
    void applyTextureSettings(unsigned int textureID, int wrapMode, bool useNearest);
    unsigned int createSolidColorTexture(float r, float g, float b, float a);
    void createMaterialTextures();

private:
    // 轨道相机参数
    float orbit_radius_ = 3.0f;           // 相机到目标的距离
    float orbit_yaw_ = 0.0f;              // 水平角度
    float orbit_pitch_ = 0.0f;            // 垂直角度
    glm::vec3 orbit_target_ = glm::vec3(0.0f, 0.0f, 0.0f);  // 观察目标点

    float model_angle_ = 0.0f;
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

    float ambient_strength_ = 0.1f;
    float specular_strength_ = 0.5;

    bool use_gouraud_ = false;              // 切换标志

    // 材质参数
    glm::vec3 material_ambient_ = glm::vec3(0.1f, 0.1f, 0.1f);
    glm::vec3 material_diffuse_ = glm::vec3(0.8f, 0.8f, 0.8f);
    glm::vec3 material_specular_ = glm::vec3(1.0f, 1.0f, 1.0f);
    float material_shininess_ = 32.0f;

    // 光源参数
    glm::vec3 light_ambient_ = glm::vec3(0.2f, 0.2f, 0.2f);
    glm::vec3 light_diffuse_ = glm::vec3(0.8f, 0.8f, 0.8f);
    glm::vec3 light_specular_ = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 light_color_ = glm::vec3(1.0f, 1.0f, 1.0f);  // 整体光照颜色

    bool use_texture_material_ = false;  // 材质贴图开关
    unsigned int diffuseMap_;             // 漫反射贴图
    unsigned int specularMap_;            // 镜面反射贴图

};

