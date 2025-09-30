#include "demo.h"
#include "ResourceManager.h"
#include "shader.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <iostream>

void ModelLoadingDemo::Initialize() {
    // 加载模型着色器
    modelShader = ResourceManager::LoadShader("model",
        "shaders/model.vs", "shaders/model.fs");
    outlineShader = ResourceManager::LoadShader("outline",
        "shaders/stenciltest/outliner.vs", "shaders/stenciltest/outliner.fs");
    screenShader = ResourceManager::LoadShader("screen",
        "shaders/framebuffer/screen.vs", "shaders/framebuffer/screen.fs");

    // 设置相机
    camera->SetType(CameraType::ORBIT);
    camera->radius = 8.0f;
    camera->target = glm::vec3(0.0f, 0.0f, 0.0f);

    // 初始化网格和坐标轴
    gridAxisHelper->Initialize();
    // 可选：根据相机距离自动调整
    gridAxisHelper->UpdateGridForCamera(camera->radius);

    // 创建基础光照
    auto dirLight = lightManager->CreateDirectionalLight();
    dirLight->direction = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.3f));
    dirLight->ambient = glm::vec3(0.3f);
    dirLight->diffuse = glm::vec3(1.0f);
    dirLight->specular = glm::vec3(1.0f);

    // 添加一个点光源
    auto pointLight = lightManager->CreatePointLight(glm::vec3(2.0f, 2.0f, 2.0f));
    pointLight->ambient = glm::vec3(0.05f, 0.05f, 0.05f);
    pointLight->diffuse = glm::vec3(0.8f, 0.8f, 0.8f);
    pointLight->specular = glm::vec3(1.0f, 1.0f, 1.0f);

    // 尝试加载默认模型
    // loadModel("models/614/GirlsFrontline LewisSSR0101.pmx");

    int width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    // framebuffer configuration
    // -------------------------
    
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    // create a color attachment texture
    glGenTextures(1, &textureColorbuffer);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer, 0);
    // create a renderbuffer object for depth and stencil attachment (we won't be sampling these)
    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height); // use a single renderbuffer object for both a depth AND stencil buffer.
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo); // now actually attach it
    // now that we actually created the framebuffer and added all attachments we want to check if it is actually complete now
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    float quadVertices[] = { // vertex attributes for a quad that fills the entire screen in Normalized Device Coordinates.
        // positions   // texCoords
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,

        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
    };
    // screen quad VAO
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    glBindVertexArray(quadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));

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

void ModelLoadingDemo::RenderScene() {
    // 这个函数包含所有场景渲染逻辑（包括你的轮廓代码）

    RenderGridAndAxis();

    if (model && modelShader && outlineShader) {
        // ===== 计算模型矩阵 =====
        glm::mat4 modelMatrix = glm::mat4(1.0f);
        modelMatrix = glm::translate(modelMatrix, modelPosition);
        modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.x), glm::vec3(1, 0, 0));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.y), glm::vec3(0, 1, 0));
        modelMatrix = glm::rotate(modelMatrix, glm::radians(modelRotation.z), glm::vec3(0, 0, 1));
        modelMatrix = glm::scale(modelMatrix, glm::vec3(modelScale));

        // ===== 步骤 1：绘制正常物体 =====
        if (enableOutline) {
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
            glStencilMask(0xFF);
        }

        modelShader->use();
        modelShader->setMat4("model", modelMatrix);
        modelShader->setMat4("view", GetViewMatrix());
        modelShader->setMat4("projection", GetProjectionMatrix());
        modelShader->setVec3("viewPos", camera->position);
        lightManager->ApplyLightsToShader(*modelShader);

        model->Draw(*modelShader);

        // ===== 步骤 2：绘制轮廓 =====
        if (enableOutline) {
            glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
            glStencilMask(0x00);
            glDisable(GL_DEPTH_TEST);
            glDepthMask(GL_FALSE);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            outlineShader->use();
            outlineShader->setMat4("model", modelMatrix);
            outlineShader->setMat4("view", GetViewMatrix());
            outlineShader->setMat4("projection", GetProjectionMatrix());
            outlineShader->setVec3("outlineColor", outline_color);
            outlineShader->setFloat("outlineAlpha", outline_alpha);
            outlineShader->setFloat("outlineWidth", outline_width);

            model->Draw(*outlineShader);

            // 恢复状态
            glStencilMask(0xFF);
            glStencilFunc(GL_ALWAYS, 0, 0xFF);
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);
            glDisable(GL_BLEND);
        }
    }
}

void ModelLoadingDemo::RenderToFramebuffer() {
    // 第一遍：渲染到帧缓冲纹理

    glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);  // 轮廓需要模板测试

    RenderScene();  // 调用完整的场景渲染（包括轮廓）

    glDisable(GL_STENCIL_TEST);
}

void ModelLoadingDemo::RenderToScreen() {
    // 第二遍：后处理并渲染到屏幕
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);  // 白色背景
    glClear(GL_COLOR_BUFFER_BIT);

    glDisable(GL_DEPTH_TEST);

    screenShader->use();
    screenShader->setInt("screenTexture", 0);
    screenShader->setInt("effectType", postProcessEffect);

    glActiveTexture(GL_TEXTURE0);

    glEnable(GL_DEPTH_TEST);
}

void ModelLoadingDemo::Render() {
    if (useFramebuffer) {
        // 步骤1: 先绑定 FBO,渲染到纹理
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);  // ← 关键!
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);

        RenderScene();  // 现在场景渲染到 textureColorbuffer 纹理了

        // 步骤2: 切换到默认帧缓冲(屏幕),绘制纹理
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        screenShader->use();
        screenShader->setInt("screenTexture", 0);
        screenShader->setInt("effectType", postProcessEffect);

        glBindVertexArray(quadVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, textureColorbuffer);  // 现在纹理有内容了!
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    else {
        // 直接渲染：场景 → 屏幕（你的原始方式）
        glClearColor(0.1f, 0.1f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glEnable(GL_STENCIL_TEST);

        RenderScene();

        glDisable(GL_STENCIL_TEST);
    }
}

void ModelLoadingDemo::RenderImGui() {
    // 当前模型信息
    ImGui::Text("Current Model: %s", currentModelPath.c_str());
    ImGui::Separator();

    // 模型加载
    static char modelPath[256] = "models/614/GirlsFrontline LewisSSR0101.pmx";
    ImGui::InputText("Model Path", modelPath, sizeof(modelPath));
    ImGui::SameLine();
    if (ImGui::Button("Load")) {
        loadModel(std::string(modelPath));
    }

    // 预设模型按钮
    ImGui::Text("Quick Load:");
    if (ImGui::Button("Backpack")) {
        strcpy_s(modelPath, "models/614/GirlsFrontline LewisSSR0101.pmx");
        loadModel("models/614/GirlsFrontline LewisSSR0101.pmx");
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

    ImGui::Separator();
    ImGui::SeparatorText("Rendering Settings");

    // 轮廓控制
    ImGui::Checkbox("Enable Outline", &enableOutline);

    if (enableOutline) {
        ImGui::Indent();
        ImGui::ColorEdit3("Outline Color", &outline_color.x);
        ImGui::SliderFloat("Outline Width", &outline_width, 0.001f, 0.1f, "%.3f");
        ImGui::SliderFloat("Outline Alpha", &outline_alpha, 0.0f, 1.0f);
        ImGui::Unindent();
    }

    ImGui::Separator();
    ImGui::SeparatorText("Post-Processing");

    // 帧缓冲控制
    ImGui::Checkbox("Enable Framebuffer", &useFramebuffer);

    if (useFramebuffer) {
        ImGui::Indent();
        ImGui::Text("Post-Processing Effect:");
        ImGui::RadioButton("None (Normal)", &postProcessEffect, 0);
        ImGui::RadioButton("Inversion", &postProcessEffect, 1);
        ImGui::RadioButton("Grayscale", &postProcessEffect, 2);
        ImGui::RadioButton("Sharpen", &postProcessEffect, 3);
        ImGui::RadioButton("Blur", &postProcessEffect, 4);
        ImGui::RadioButton("Edge Detection", &postProcessEffect, 5);
        ImGui::Unindent();

        // ImGui::Text("Framebuffer: %dx%d", framebuffer->width, framebuffer->height);
    }
}

void ModelLoadingDemo::Cleanup() {
    // 资源会在ResourceManager中自动清理
    model = nullptr;
}

std::string ModelLoadingDemo::GetName() const {
    return "Model Loading";
}
