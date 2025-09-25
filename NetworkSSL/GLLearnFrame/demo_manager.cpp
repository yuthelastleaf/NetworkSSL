#include "demo.h"
#include <imgui.h>
#include <iostream>

void DemoManager::RegisterDemo(const std::string& name, std::unique_ptr<Demo> demo) {
    demos[name] = std::move(demo);
}

void DemoManager::SetCurrentDemo(const std::string& name) {
    if (demos.find(name) != demos.end()) {
        if (currentDemo) {
            currentDemo->Cleanup();
        }
        currentDemo = demos[name].get();
        currentDemoName = name;
        currentDemo->Initialize();
        std::cout << "Switched to: " << name << std::endl;
    }
    else {
        std::cout << "Demo not found: " << name << std::endl;
    }
}

void DemoManager::Update(struct GLFWwindow* window, float deltaTime) {
    if (currentDemo) {
        currentDemo->ProcInput(window, deltaTime);
        currentDemo->Update(deltaTime);
    }
}

void DemoManager::Render() {
    if (currentDemo) {
        currentDemo->Render();
    }
}

void DemoManager::RenderImGui() {
    // Demo ѡ����
    ImGui::Begin("Demo Selector");
    ImGui::Text("Current Demo: %s", currentDemoName.c_str());
    ImGui::Separator();

    for (const auto& pair : demos) {
        if (ImGui::Button(pair.first.c_str())) {
            SetCurrentDemo(pair.first);
        }
    }
    ImGui::End();

    // ��ǰ Demo �Ŀ������
    if (currentDemo) {
        currentDemo->RenderImGui();
    }
}

std::vector<std::string> DemoManager::GetDemoNames() const {
    std::vector<std::string> names;
    for (const auto& pair : demos) {
        names.push_back(pair.first);
    }
    return names;
}

void DemoManager::Cleanup() {
    if (currentDemo) {
        currentDemo->Cleanup();
    }
}