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

// Demo���� - ���ڰ�������͹���ϵͳ
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

    // ����͹��շ��ʽӿ�
    Camera* GetCamera() { return camera.get(); }
    LightManager* GetLightManager() { return lightManager.get(); }

protected:
    // ͨ�õ�ImGui��Ⱦ����
    void RenderCameraControls();
    void RenderLightControls();
    void RenderGridAndAxis();
    void RenderGridAxisControls();

    // ��ȡ���þ���
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetBackViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;

private:
    void handleMouseInput(float deltaTime);
    void handleKeyboardInput(GLFWwindow* window, float deltaTime);
};


// ģ�ͼ�����ʾ
class ModelLoadingDemo : public Demo {
private:
    std::shared_ptr<Model> model;
    std::shared_ptr<Shader> modelShader;
    std::shared_ptr<Shader> outlineShader;
    std::string currentModelPath = "None";

    // ģ�ͱ任����
    glm::vec3 modelPosition = glm::vec3(0.0f);
    glm::vec3 modelRotation = glm::vec3(0.0f);
    float modelScale = 1.0f;

    // ��������
    bool autoRotate = false;
    float rotationSpeed = 45.0f; // degrees per second

    // ֡�������
    std::shared_ptr<Shader> screenShader;
    std::shared_ptr<Shader> skyboxShader;
    unsigned int fbo = 0;
    unsigned int rfbo = 0;
    unsigned int textureColorbuffer = 0;
    unsigned int rtextureColorbuffer = 0;
    unsigned int rbo;
    unsigned int rrbo;
    unsigned int quadVAO, quadVBO;
    unsigned int rquadVAO, rquadVBO;
    unsigned int skyboxVAO, skyboxVBO;
    unsigned int sky_texture;

    // ����ѡ��
    bool useFramebuffer = false;     // �Ƿ�ʹ��֡����
    bool enableOutline = true;       // �Ƿ��������
    int postProcessEffect = 0;       // ����Ч��

    glm::vec3 outline_color = glm::vec3(1.0f, 0.5f, 0.0f);
    float outline_alpha = 0.5f;
    float outline_width = 0.02f;

    bool enableReflection = false;
    float reflectionStrength = 0.5f;

    // ����
    bool enableRefraction = false;
    float refractionRatio = 1.0f / 1.52f;  // Ĭ�ϲ���������

public:
    void Initialize() override;
    void Update(float deltaTime) override;
    void Render() override;
    void RenderImGui() override;
    void Cleanup() override;
    std::string GetName() const override;

private:
    void loadModel(const std::string& path);

    // ��������
    void RenderScene(bool reverse = false);  // ��Ⱦ���������ã�
    void RenderToFramebuffer();  // ��Ⱦ��֡����
    void RenderToScreen();  // ��Ⱦ����Ļ
    void RenderSkybox(bool reverse = false);
};

// Demo������
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

// ��Ӧ����
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