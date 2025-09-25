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

    // ��ֹ����
    MouseManager(const MouseManager&) = delete;
    MouseManager& operator=(const MouseManager&) = delete;

    // GLFW�ص�����
    static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
        Get().handleCursorPos(xpos, ypos);
    }

    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
        Get().handleMouseButton(button, action, mods);
    }

    // ���������ֻص�
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
        Get().handleScroll(xoffset, yoffset);
    }

    // �����ӿ�
    void RegisterCallbacks(GLFWwindow* window) {
        glfwSetCursorPosCallback(window, CursorPosCallback);
        glfwSetMouseButtonCallback(window, MouseButtonCallback);
        glfwSetScrollCallback(window, ScrollCallback);  // �������ֻص�ע��
    }

    void ResetDeltas() {
        deltaX = 0.0;
        deltaY = 0.0;
        scrollX = 0.0;  // ���������ù�������
        scrollY = 0.0;
    }

    // ��ȡԭʼ�������
    glm::vec2 GetMousePos() const { return glm::vec2(currentX, currentY); }
    glm::vec2 GetMouseDelta() const { return glm::vec2(deltaX, deltaY); }
    bool IsFirstMouse() const { return firstMouse; }

    // ��������ȡ��������
    glm::vec2 GetScrollDelta() const { return glm::vec2(scrollX, scrollY); }
    // ��ȡ����ֵ���Զ�����
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

    // ��갴��״̬��ѯ
    bool IsMouseButtonPressed(int button) const {
        if (button >= 0 && button < 8) {
            return mouseButtons[button];
        }
        return false;
    }

    // ��ݵİ�����ѯ����
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
            deltaY = currentY - y;  // Y�ᷭת
            currentX = x;
            currentY = y;
        }
    }

    void handleMouseButton(int button, int action, int mods) {
        if (button >= 0 && button < 8) {
            mouseButtons[button] = (action == GLFW_PRESS);
        }
    }

    // ������������ֻص�
    void handleScroll(double xoffset, double yoffset) {
        scrollX = xoffset;
        scrollY = yoffset;
    }

    // ��Ա����
    double currentX = 0.0, currentY = 0.0;
    double deltaX = 0.0, deltaY = 0.0;
    bool firstMouse = true;
    std::array<bool, 8> mouseButtons = {};

    // ����������״̬
    double scrollX = 0.0, scrollY = 0.0;  // ��������
};
