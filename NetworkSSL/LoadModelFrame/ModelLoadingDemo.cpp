#include "demo.h"
#include "ResourceManager.h"
#include "shader.h"
#include <glad/glad.h>
#include <imgui.h>
#include <iostream>

void ModelLoadingDemo::Initialize() {
    // 加载模型着色器
    modelShader = ResourceManager::LoadShader("model",
        "shaders/model.vs", "shaders/model.fs");

    // 设置相机
    camera->SetType(CameraType::ORBIT);
    camera->radius = 8.0f;
    camera->target = glm::vec3(0.0f, 0.0f, 0.0f);

    // 创建基础光照
    auto dirLight = lightManager->CreateDirectionalLight();
    dirLight->direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    dirLight->ambient = glm::vec3(0.05f, 0.05f, 0.05f);
    dirLight->diffuse = glm::vec3(0.4f, 0.4f, 0.4f);
    dirLight->specular = glm::vec3(0.5f, 0.5f, 0.5f);

    // 添加一个点光源
    auto pointLight = lightManager->CreatePointLight(glm::vec3(2.0f, 2.0f, 2.0f));
    pointLight->ambient = glm::vec3(0.05f, 0.05f, 0.05f);
    pointLight->diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
    pointLight->specular = glm::vec3(1.0f, 1.0f, 1.0f);

    // 尝试加载默认模型
    loadModel("models/backpack/backpack.obj");

    std::cout << "ModelLoadingDemo initialized" << std::endl;
}

void ModelLoadingDemo::loadModel(const std::string& path) {
    try {
        model = ResourceManager::LoadModel("currentModel", path);
        currentModelPath = path;
        std::cout << "Successfully loaded model: " << path << std::endl;
    }
    catch (const std::exception& e) {
        std::cout << "Failed to load model: " << path << " - " << e.what() << std::endl;
        model = nullptr;
        currentModelPath = "None";
    }
}

void ModelLoadingDemo::Update(float deltaTime) {
    // 可以添加模型动画或其他更新逻辑
    if (autoRotate && model) {
        static float rotation = 0.0f;
        rotation += rotationSpeed * deltaTime;
        if (rotation > 360.0f) rotation -= 360.0f;
        modelRotation.y = rotation;
    }
}

void ModelLoadingDemo::Render() {
    glEnable(GL_DEPTH_TEST);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (model && modelShader) {
        modelShader->use();
        modelShader->setMat4("view", GetViewMatrix());
        modelShader->setMat4("projection", GetProjectionMatrix());
        modelShader->setVec3("viewPos", camera->position);

        // 应用光照
        lightManager->ApplyLightsToShader(*modelShader);

        // 计算模型变换矩阵
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, modelPosition);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.x), glm::vec3(1, 0, 0));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.y), glm::vec3(0, 1, 0));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.z), glm::vec3(0, 0, 1));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(modelScale));

        modelShader->setMat4("model", modelMatrix);

        // 渲染模型
        model->Draw(*modelShader);
    }
}

void ModelLoadingDemo::RenderImGui() {
    RenderCameraControls();
    RenderLightControls();

    ImGui::Begin("Model Loading Controls");

    // 当前模型信息
    ImGui::Text("Current Model: %s", currentModelPath.c_str());
    ImGui::Separator();

    // 模型加载
    static char modelPath[256] = "models/backpack/backpack.obj";
    ImGui::InputText("Model Path", modelPath, sizeof(modelPath));
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        loadModel(std::string(modelPath));
    }

    // 预设模型按钮
    ImGui::Text("Quick Load:");
    if (ImGui::Button("Backpack")) {
        strcpy_s(modelPath, "models/backpack/backpack.obj");
        loadModel("models/backpack/backpack.obj");
    }
    ImGui::SameLine();
    if (ImGui::Button("Nanosuit")) {
        strcpy_s(modelPath, "models/nanosuit/nanosuit.obj");
        loadModel("models/nanosuit/nanosuit.obj");
    }
    ImGui::SameLine();
    if (ImGui::Button("Sponza")) {
        strcpy_s(modelPath, "models/sponza/sponza.obj");
        loadModel("models/sponza/sponza.obj");
    }

    if (model) {
        ImGui::Separator();
        ImGui::Text("Model Transform:");

        // 位置控制
        ImGui::SliderFloat3("Position", &modelPosition.x, -10.0f, 10.0f);

        // 旋转控制
        ImGui::SliderFloat3("Rotation", &modelRotation.x, 0.0f, 360.0f);

        // 缩放控制
        ImGui::SliderFloat("Scale", &modelScale, 0.1f, 5.0f);

        // 自动旋转
        ImGui::Checkbox("Auto Rotate", &autoRotate);
        if (autoRotate) {
            ImGui::SliderFloat("Rotation Speed", &rotationSpeed, 10.0f, 180.0f);
        }

        // 重置按钮
        if (ImGui::Button("Reset Transform")) {
            modelPosition = glm::vec3(0.0f);
            modelRotation = glm::vec3(0.0f);
            modelScale = 1.0f;
        }
    }

    ImGui::End();
}

void ModelLoadingDemo::Cleanup() {
    // 资源会在ResourceManager中自动清理
    model = nullptr;
}

std::string ModelLoadingDemo::GetName() const {
    return "Model Loading";
}
