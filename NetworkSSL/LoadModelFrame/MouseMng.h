#pragma once
#include <iostream>
#include <array>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <string>

class MouseManager {
public:
    static MouseManager& Get() {
        static MouseManager instance;
        return instance;
    }

    MouseManager(const MouseManager&) = delete;
    MouseManager& operator=(const MouseManager&) = delete;

    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
        Get().handleCursorPos(xpos, ypos);
    }

    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        Get().handleMouseButton(button, action, mods);
    }

    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        Get().handleScroll(xoffset, yoffset);
    }

    void RegisterCallbacks(GLFWwindow* window) {
        glfwSetCursorPosCallback(window, CursorPosCallback);
        glfwSetMouseButtonCallback(window, MouseButtonCallback);
        glfwSetScrollCallback(window, ScrollCallback);
    }

    void ResetDeltas() {
        deltaX = 0.0;
        deltaY = 0.0;
        scrollX = 0.0;
        scrollY = 0.0;
    }

    glm::vec2 GetMousePos() const { return glm::vec2(currentX, currentY); }
    glm::vec2 GetMouseDelta() const { return glm::vec2(deltaX, deltaY); }
    bool IsFirstMouse() const { return firstMouse; }
    glm::vec2 GetScrollDelta() const { return glm::vec2(scrollX, scrollY); }

    float GetScrollY() {
        float result = static_cast<float>(scrollY);
        scrollY = 0.0;
        return result;
    }

    float GetScrollX() {
        float result = static_cast<float>(scrollX);
        scrollX = 0.0;
        return result;
    }

    bool IsMouseButtonPressed(int button) const {
        if (button >= 0 && button < 8) {
            return mouseButtons[button];
        }
        return false;
    }

    bool IsLeftButtonPressed() const { return IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT); }
    bool IsRightButtonPressed() const { return IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT); }
    bool IsMiddleButtonPressed() const { return IsMouseButtonPressed(GLFW_MOUSE_BUTTON_MIDDLE); }

private:
    MouseManager() = default;

    void handleCursorPos(double x, double y) {
        // 🔥 关键：检查 ImGui 是否占用鼠标
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) {
            firstMouse = true;  // 重置状态，避免下次移动时跳跃
            deltaX = 0.0;
            deltaY = 0.0;
            return;
        }

        if (firstMouse) {
            currentX = x;
            currentY = y;
            firstMouse = false;
            deltaX = 0.0;
            deltaY = 0.0;
        }
        else {
            deltaX = x - currentX;
            deltaY = currentY - y;  // Y轴翻转
            currentX = x;
            currentY = y;
        }
    }

    void handleMouseButton(int button, int action, int mods) {
        // 🔥 关键：检查 ImGui 是否占用鼠标
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) {
            return;
        }

        if (button >= 0 && button < 8) {
            mouseButtons[button] = (action == GLFW_PRESS);
        }
    }

    void handleScroll(double xoffset, double yoffset) {
        // 🔥 关键：检查 ImGui 是否占用鼠标
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) {
            return;
        }

        scrollX = xoffset;
        scrollY = yoffset;
    }

    double currentX = 0.0, currentY = 0.0;
    double deltaX = 0.0, deltaY = 0.0;
    bool firstMouse = true;
    std::array<bool, 8> mouseButtons = {};
    double scrollX = 0.0, scrollY = 0.0;
};