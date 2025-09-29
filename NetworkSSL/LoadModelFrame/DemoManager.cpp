#include "demo.h"
#include <imgui.h>
#include <iostream>

void DemoManager::RegisterDemo(const std::string& name, std::unique_ptr<Demo> demo) {
    demos[name] = std::move(demo);
    std::cout << "Registered demo: " << name << std::endl;
}

void DemoManager::SetCurrentDemo(const std::string& name) {
    auto it = demos.find(name);
    if (it != demos.end()) {
        // 清理当前Demo
        if (currentDemo) {
            currentDemo->Cleanup();
        }

        // 设置新Demo
        currentDemo = it->second.get();
        currentDemoName = name;

        // 初始化新Demo
        try {
            currentDemo->Initialize();
            std::cout << "Switched to demo: " << name << std::endl;
        }
        catch (const std::exception& e) {
            std::cout << "Failed to initialize demo " << name << ": " << e.what() << std::endl;
            currentDemo = nullptr;
            currentDemoName = "";
        }
    }
    else {
        std::cout << "Demo not found: " << name << std::endl;
    }
}

void DemoManager::Update(GLFWwindow* window, float deltaTime) {
    if (currentDemo) {
        currentDemo->ProcessInput(window, deltaTime);
        currentDemo->Update(deltaTime);
    }
}

void DemoManager::Render() {
    if (currentDemo) {
        currentDemo->Render();
    }
}

void DemoManager::RenderImGui() {
    // Demo选择器
    ImGui::Begin("Demo Selector");
    ImGui::Text("Current Demo: %s", currentDemoName.c_str());
    ImGui::Separator();

    for (const auto& [name, demo] : demos) {
        bool isSelected = (name == currentDemoName);
        if (ImGui::Selectable(name.c_str(), isSelected)) {
            if (!isSelected) {
                SetCurrentDemo(name);
            }
        }
    }
    ImGui::End();

    // 当前Demo的控制面板
    if (currentDemo) {
        currentDemo->RenderControlPanel();
    }
}

std::vector<std::string> DemoManager::GetDemoNames() const {
    std::vector<std::string> names;
    for (const auto& [name, demo] : demos) {
        names.push_back(name);
    }
    return names;
}

void DemoManager::Cleanup() {
    if (currentDemo) {
        currentDemo->Cleanup();
        currentDemo = nullptr;
    }
}