#pragma once

#include "demo.h"

class TransformDemo : public Demo {
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
    // 变换参数
    float rotationAngle = 0.0f;
    float scaleX = 1.0f, scaleY = 1.0f;
    float translateX = 0.0f, translateY = 0.0f;
    bool autoRotate = false;
    float rotationSpeed = 1.0f;
};
