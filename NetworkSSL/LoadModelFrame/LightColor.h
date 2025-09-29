#pragma once
#include "demo.h"

class LightColorDemo : public Demo {
private:
    unsigned int VBO, VAO;
    unsigned int lightVAO;
    std::shared_ptr<Shader> phongShader;
    std::shared_ptr<Shader> gouraudShader;
    std::shared_ptr<Shader> lightCubeShader;

    unsigned int diffuseMap;
    unsigned int specularMap;

    float model_angle_ = -55.0f;
    bool auto_rotate_ = false;
    float rotate_speed_ = 1.0f;

    glm::vec3 materialAmbient = glm::vec3(1.0f, 0.5f, 0.31f);
    float materialShininess = 32.0f;

    bool use_gouraud_ = false;
    bool showLightCubes = true;

public:
    void Initialize() override;
    void Update(float deltaTime) override;
    void Render() override;
    void RenderImGui() override;
    void Cleanup() override;
    std::string GetName() const override;

private:
    void createTextures();
    void setupGeometry();
    unsigned int loadTextureFromFile(const char* path);
    unsigned int createFallbackTexture(unsigned char r, unsigned char g, unsigned char b);
};
