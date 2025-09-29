#include "demo.h"
#include "MouseMng.h"
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/matrix_transform.hpp>

Demo::Demo() {
    camera = std::make_unique<Camera>(CameraType::ORBIT);
    lightManager = std::make_unique<LightManager>();
    gridAxisHelper = std::make_shared<GridAxisHelper>();
}

void Demo::ProcessInput(GLFWwindow* window, float deltaTime) {
    handleMouseInput(deltaTime);
    handleKeyboardInput(window, deltaTime);
}

glm::mat4 Demo::GetViewMatrix() const {
    return camera->GetViewMatrix();
}

glm::mat4 Demo::GetProjectionMatrix() const {
    int width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    float aspectRatio = (width > 0 && height > 0) ? (float)width / (float)height : 1.0f;
    return camera->GetProjectionMatrix(aspectRatio);
}

void Demo::RenderCameraControls() {
    ImGui::Begin("Camera Controls");

    const char* cameraTypes[] = { "Orbit", "FPS", "Free" };
    int currentType = (int)camera->GetType();
    if (ImGui::Combo("Camera Type", &currentType, cameraTypes, 3)) {
        camera->SetType((CameraType)currentType);
    }

    if (camera->type == CameraType::ORBIT) {
        ImGui::SliderFloat("Orbit Radius", &camera->radius, 1.0f, 50.0f);
        ImGui::SliderFloat("Orbit Yaw", &camera->yaw, -180.0f, 180.0f);
        ImGui::SliderFloat("Orbit Pitch", &camera->pitch, -89.0f, 89.0f);
        ImGui::SliderFloat3("Target", &camera->target.x, -10.0f, 10.0f);
    }

    ImGui::SliderFloat("FOV", &camera->fov, 1.0f, 90.0f);
    ImGui::SliderFloat("Move Speed", &camera->moveSpeed, 0.1f, 10.0f);
    ImGui::SliderFloat("Mouse Sensitivity", &camera->mouseSensitivity, 10.0f, 500.0f);

    ImGui::End();
}

void Demo::RenderLightControls() {
    ImGui::Begin("Light Controls");

    // ========== 添加灯光按钮 ==========
    ImGui::Text("Add Lights:");
    if (ImGui::Button("Add Directional Light")) {
        auto light = lightManager->CreateDirectionalLight();
        light->direction = glm::vec3(-0.2f, -1.0f, -0.3f);
        light->ambient = glm::vec3(0.05f);
        light->diffuse = glm::vec3(0.4f);
        light->specular = glm::vec3(0.5f);
        std::cout << "Created Directional Light" << std::endl;
    }
    ImGui::SameLine();

    if (ImGui::Button("Add Point Light")) {
        auto light = lightManager->CreatePointLight(glm::vec3(2.0f, 2.0f, 2.0f));
        light->ambient = glm::vec3(0.05f);
        light->diffuse = glm::vec3(0.8f);
        light->specular = glm::vec3(1.0f);
        std::cout << "Created Point Light at (2, 2, 2)" << std::endl;
    }
    ImGui::SameLine();

    if (ImGui::Button("Add Spot Light")) {
        auto light = lightManager->CreateSpotLight(
            glm::vec3(0.0f, 3.0f, 0.0f),
            glm::vec3(0.0f, -1.0f, 0.0f));
        light->ambient = glm::vec3(0.0f);
        light->diffuse = glm::vec3(1.0f);
        light->specular = glm::vec3(1.0f);
        std::cout << "Created Spot Light at (0, 3, 0)" << std::endl;
    }

    if (ImGui::Button("Clear All Lights")) {
        lightManager->Clear();
        std::cout << "Cleared all lights" << std::endl;
    }

    ImGui::Separator();
    ImGui::Text("Current Lights: %zu", lightManager->GetLights().size());

    // ========== 灯光列表 ==========
    const auto& lights = lightManager->GetLights();
    for (int i = 0; i < lights.size(); ++i) {
        auto& light = lights[i];
        std::string label = "Light " + std::to_string(i);

        ImGui::PushID(i);

        if (ImGui::TreeNode(label.c_str())) {
            ImGui::Checkbox("Enabled", &light->enabled);

            const char* typeNames[] = { "Directional", "Point", "Spot" };
            ImGui::Text("Type: %s", typeNames[(int)light->type]);

            ImGui::Separator();

            // ========== 位置（点光源和聚光灯）==========
            if (light->type != LightType::DIRECTIONAL) {
                ImGui::Text("Position:");
                if (ImGui::DragFloat3("##pos", &light->position.x, 0.1f, -50.0f, 50.0f, "%.2f")) {
                    std::cout << "Light " << i << " position changed to ("
                        << light->position.x << ", "
                        << light->position.y << ", "
                        << light->position.z << ")" << std::endl;
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Drag to adjust | Ctrl+Click to type precise values");
                }
            }

            // ========== 方向（方向光和聚光灯）==========
            if (light->type != LightType::POINT) {
                ImGui::Text("Direction:");
                if (ImGui::DragFloat3("##dir", &light->direction.x, 0.01f, -1.0f, 1.0f, "%.3f")) {
                    std::cout << "Light " << i << " direction changed" << std::endl;
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Drag to adjust | Ctrl+Click to type");
                }
            }

            ImGui::Separator();

            // ========== 光照颜色 ==========
            ImGui::Text("Ambient:");
            ImGui::ColorEdit3("##ambient", &light->ambient.x, ImGuiColorEditFlags_Float);

            ImGui::Text("Diffuse:");
            ImGui::ColorEdit3("##diffuse", &light->diffuse.x, ImGuiColorEditFlags_Float);

            ImGui::Text("Specular:");
            ImGui::ColorEdit3("##specular", &light->specular.x, ImGuiColorEditFlags_Float);

            // ========== 点光源衰减参数 ==========
            if (light->type == LightType::POINT) {
                ImGui::Separator();
                ImGui::Text("Attenuation:");

                ImGui::DragFloat("Constant", &light->constant, 0.01f, 0.0f, 2.0f, "%.4f");
                ImGui::DragFloat("Linear", &light->linear, 0.001f, 0.0f, 1.0f, "%.4f");
                ImGui::DragFloat("Quadratic", &light->quadratic, 0.0001f, 0.0f, 1.0f, "%.5f");

                // 预设按钮
                ImGui::Spacing();
                ImGui::Text("Distance Presets:");
                if (ImGui::Button("7 units")) {
                    light->constant = 1.0f;
                    light->linear = 0.7f;
                    light->quadratic = 1.8f;
                }
                ImGui::SameLine();
                if (ImGui::Button("13 units")) {
                    light->constant = 1.0f;
                    light->linear = 0.35f;
                    light->quadratic = 0.44f;
                }
                ImGui::SameLine();
                if (ImGui::Button("20 units")) {
                    light->constant = 1.0f;
                    light->linear = 0.22f;
                    light->quadratic = 0.20f;
                }
                if (ImGui::Button("32 units")) {
                    light->constant = 1.0f;
                    light->linear = 0.14f;
                    light->quadratic = 0.07f;
                }
                ImGui::SameLine();
                if (ImGui::Button("50 units")) {
                    light->constant = 1.0f;
                    light->linear = 0.09f;
                    light->quadratic = 0.032f;
                }
                ImGui::SameLine();
                if (ImGui::Button("100 units")) {
                    light->constant = 1.0f;
                    light->linear = 0.045f;
                    light->quadratic = 0.0075f;
                }
            }

            // ========== 聚光灯角度参数 ==========
            if (light->type == LightType::SPOT) {
                ImGui::Separator();
                ImGui::Text("Spotlight Cone:");

                // 转换为度数方便理解
                float cutOffDegrees = glm::degrees(acos(light->cutOff));
                float outerCutOffDegrees = glm::degrees(acos(light->outerCutOff));

                // Inner Cut Off
                if (ImGui::DragFloat("Inner Angle", &cutOffDegrees, 0.5f, 0.0f, 89.0f, "%.1f°")) {
                    light->cutOff = cos(glm::radians(cutOffDegrees));
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Inner cone angle (full brightness)");
                }

                // Outer Cut Off
                if (ImGui::DragFloat("Outer Angle", &outerCutOffDegrees, 0.5f, 0.0f, 90.0f, "%.1f°")) {
                    light->outerCutOff = cos(glm::radians(outerCutOffDegrees));
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Outer cone angle (fade to dark)");
                }

                // 确保外角大于内角
                if (outerCutOffDegrees < cutOffDegrees) {
                    outerCutOffDegrees = cutOffDegrees;
                    light->outerCutOff = cos(glm::radians(outerCutOffDegrees));
                }

                // 聚光灯也有衰减参数
                ImGui::Separator();
                ImGui::Text("Attenuation:");
                ImGui::DragFloat("Constant##spot", &light->constant, 0.01f, 0.0f, 2.0f, "%.4f");
                ImGui::DragFloat("Linear##spot", &light->linear, 0.001f, 0.0f, 1.0f, "%.4f");
                ImGui::DragFloat("Quadratic##spot", &light->quadratic, 0.0001f, 0.0f, 1.0f, "%.5f");
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    ImGui::End();
}


void Demo::handleMouseInput(float deltaTime) {
    if (MouseManager::Get().IsLeftButtonPressed()) {
        glm::vec2 mouseDelta = MouseManager::Get().GetMouseDelta();
        camera->ProcessMouseMovement(mouseDelta.x, mouseDelta.y);
    }

    float scrollDelta = MouseManager::Get().GetScrollY();
    if (abs(scrollDelta) > 0.001f) {
        camera->ProcessMouseScroll(scrollDelta);
    }
}

void Demo::handleKeyboardInput(GLFWwindow* window, float deltaTime) {
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera->ProcessKeyboard(3, deltaTime);
}

// 辅助方法
void Demo::RenderGridAndAxis() {
    if (gridAxisHelper) {
        gridAxisHelper->Render(GetViewMatrix(), GetProjectionMatrix());
    }
}

void Demo::RenderGridAxisControls() {
    if (!gridAxisHelper) return;

    ImGui::Begin("Grid & Axis");

    auto& gridConfig = gridAxisHelper->GetGridConfig();
    auto& axisConfig = gridAxisHelper->GetAxisConfig();

    ImGui::Checkbox("Show Grid", &gridConfig.enabled);
    if (gridConfig.enabled) {
        if (ImGui::SliderFloat("Grid Size", &gridConfig.size, 5.0f, 100.0f)) {
            gridAxisHelper->SetGridSize(gridConfig.size);
        }
        if (ImGui::SliderFloat("Grid Spacing", &gridConfig.spacing, 0.1f, 10.0f)) {
            gridAxisHelper->SetGridSpacing(gridConfig.spacing);
        }
        ImGui::ColorEdit3("Grid Color", &gridConfig.color[0]);
        ImGui::SliderFloat("Line Width", &gridConfig.lineWidth, 0.5f, 5.0f);
    }

    ImGui::Separator();
    ImGui::Checkbox("Show Axis", &axisConfig.enabled);
    if (axisConfig.enabled) {
        if (ImGui::SliderFloat("Axis Length", &axisConfig.length, 1.0f, 20.0f)) {
            gridAxisHelper->SetAxisLength(axisConfig.length);
        }
        ImGui::ColorEdit3("X-Axis Color", &axisConfig.xColor[0]);
        ImGui::ColorEdit3("Y-Axis Color", &axisConfig.yColor[0]);
        ImGui::ColorEdit3("Z-Axis Color", &axisConfig.zColor[0]);
    }

    ImGui::End();
}

