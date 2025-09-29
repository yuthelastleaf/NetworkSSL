#pragma once
#include <string>
#include <memory>
#include <map>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Camera.h"
#include "Light.h"
#include "GridAxisHelper.h"
#include "Model.h"
#include "shader.h"

struct GLFWwindow;

// Demo基类 - 现在包含相机和光照系统
class Demo {
protected:
    std::unique_ptr<Camera> camera;
    std::unique_ptr<LightManager> lightManager;
    std::shared_ptr<GridAxisHelper> gridAxisHelper;

public:
    Demo();
    virtual ~Demo() = default;

    virtual void Initialize() = 0;
    virtual void Update(float deltaTime) = 0;
    virtual void Render() = 0;
    virtual void RenderImGui() = 0;
    virtual void Cleanup() = 0;
    virtual std::string GetName() const = 0;
    virtual void ProcessInput(GLFWwindow* window, float deltaTime);
    void RenderControlPanel();

    // 相机和光照访问接口
    Camera* GetCamera() { return camera.get(); }
    LightManager* GetLightManager() { return lightManager.get(); }

protected:
    // 通用的ImGui渲染函数
    void RenderCameraControls();
    void RenderLightControls();
    void RenderGridAndAxis();
    void RenderGridAxisControls();

    // 获取常用矩阵
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;

private:
    void handleMouseInput(float deltaTime);
    void handleKeyboardInput(GLFWwindow* window, float deltaTime);
};


// 模型加载演示
class ModelLoadingDemo : public Demo {
private:
    std::shared_ptr<Model> model;
    std::shared_ptr<Shader> modelShader;
    std::string currentModelPath = "None";

    // 模型变换参数
    glm::vec3 modelPosition = glm::vec3(0.0f);
    glm::vec3 modelRotation = glm::vec3(0.0f);
    float modelScale = 1.0f;

    // 动画参数
    bool autoRotate = false;
    float rotationSpeed = 45.0f; // degrees per second

public:
    void Initialize() override;
    void Update(float deltaTime) override;
    void Render() override;
    void RenderImGui() override;
    void Cleanup() override;
    std::string GetName() const override;

private:
    void loadModel(const std::string& path);
};

// Demo管理器
class DemoManager {
private:
    std::map<std::string, std::unique_ptr<Demo>> demos;
    Demo* currentDemo = nullptr;
    std::string currentDemoName;

public:
    void RegisterDemo(const std::string& name, std::unique_ptr<Demo> demo);
    void SetCurrentDemo(const std::string& name);
    void Update(GLFWwindow* window, float deltaTime);
    void Render();
    void RenderImGui();
    std::vector<std::string> GetDemoNames() const;
    void Cleanup();
};

// 主应用类
class OpenGLApp {
private:
    GLFWwindow* window;
    DemoManager demoManager;
    float lastFrame = 0.0f;

public:
    bool Initialize();
    void Run(const std::string& initialDemo = "");
    void Cleanup();
};