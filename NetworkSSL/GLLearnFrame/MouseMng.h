#pragma once
#include <iostream>
#include <array>
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <string>


class MouseManager {
public:
    static MouseManager& Get() {
        static MouseManager instance;
        return instance;
    }

    // 禁止拷贝
    MouseManager(const MouseManager&) = delete;
    MouseManager& operator=(const MouseManager&) = delete;

    // GLFW回调函数
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
        Get().handleCursorPos(xpos, ypos);
    }

    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        Get().handleMouseButton(button, action, mods);
    }

    // 新增：滚轮回调
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        Get().handleScroll(xoffset, yoffset);
    }

    // 公共接口
    void RegisterCallbacks(GLFWwindow* window) {
        glfwSetCursorPosCallback(window, CursorPosCallback);
        glfwSetMouseButtonCallback(window, MouseButtonCallback);
        glfwSetScrollCallback(window, ScrollCallback);  // 新增滚轮回调注册
    }

    void ResetDeltas() {
        deltaX = 0.0;
        deltaY = 0.0;
        scrollX = 0.0;  // 新增：重置滚轮增量
        scrollY = 0.0;
    }

    // 获取原始鼠标数据
    glm::vec2 GetMousePos() const { return glm::vec2(currentX, currentY); }
    glm::vec2 GetMouseDelta() const { return glm::vec2(deltaX, deltaY); }
    bool IsFirstMouse() const { return firstMouse; }

    // 新增：获取滚轮数据
    glm::vec2 GetScrollDelta() const { return glm::vec2(scrollX, scrollY); }
    // 获取滚轮值并自动重置
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

    // 鼠标按键状态查询
    bool IsMouseButtonPressed(int button) const {
        if (button >= 0 && button < 8) {
            return mouseButtons[button];
        }
        return false;
    }

    // 便捷的按键查询方法
    bool IsLeftButtonPressed() const { return IsMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT); }
    bool IsRightButtonPressed() const { return IsMouseButtonPressed(GLFW_MOUSE_BUTTON_RIGHT); }
    bool IsMiddleButtonPressed() const { return IsMouseButtonPressed(GLFW_MOUSE_BUTTON_MIDDLE); }

private:
    MouseManager() = default;

    void handleCursorPos(double x, double y) {
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
        if (button >= 0 && button < 8) {
            mouseButtons[button] = (action == GLFW_PRESS);
        }
    }

    // 新增：处理滚轮回调
    void handleScroll(double xoffset, double yoffset) {
        scrollX = xoffset;
        scrollY = yoffset;
    }

    // 成员变量
    double currentX = 0.0, currentY = 0.0;
    double deltaX = 0.0, deltaY = 0.0;
    bool firstMouse = true;
    std::array<bool, 8> mouseButtons = {};

    // 新增：滚轮状态
    double scrollX = 0.0, scrollY = 0.0;  // 滚轮增量
};
