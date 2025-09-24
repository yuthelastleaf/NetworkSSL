#pragma once
#include <string>
#include <memory>
#include <map>
#include <vector>

#include "shader.h"

// Demo 基类
class Demo {
public:
    virtual ~Demo() = default;
    virtual void Initialize() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    virtual void RenderImGui() = 0;
    virtual void Cleanup() = 0;
    virtual std::string GetName() const = 0;
};

// 三角形 Demo
class TriangleDemo : public Demo {
private:
    unsigned int VBO, VAO;
    std::unique_ptr<Shader> shader;

public:
    void Initialize() override;
    void Update(float deltaTime) override;
    void Render() override;
    void RenderImGui() override;
    void Cleanup() override;
    std::string GetName() const override;
};

// 纹理 Demo
class TextureDemo : public Demo {
private:
    unsigned int VBO, VAO, EBO;
    std::unique_ptr<Shader> shader;
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
};

// Demo 管理器
class DemoManager {
private:
    std::map<std::string, std::unique_ptr<Demo>> demos;
    Demo* currentDemo = nullptr;
    std::string currentDemoName;

public:
    void RegisterDemo(const std::string& name, std::unique_ptr<Demo> demo);
    void SetCurrentDemo(const std::string& name);
    void Update(float deltaTime);
    void Render();
    void RenderImGui();
    std::vector<std::string> GetDemoNames() const;
    void Cleanup();
};

// 主应用类
class OpenGLApp {
private:
    struct GLFWwindow* window;
    DemoManager demoManager;
    float lastFrame = 0.0f;

public:
    bool Initialize();
    void Run(const std::string& initialDemo = "");
    void Cleanup();
};