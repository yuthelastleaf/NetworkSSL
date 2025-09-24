#pragma once

#include "demo.h"

class in3dDemo : public Demo {
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



};

