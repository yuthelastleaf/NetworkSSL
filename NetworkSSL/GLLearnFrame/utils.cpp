#include "utils.h"
#include <GLFW/glfw3.h>
#include <iostream>

// 常量定义
const unsigned int SCR_WIDTH = 1200;
const unsigned int SCR_HEIGHT = 800;

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
}

void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

void printUsage() {
    std::cout << "OpenGL Learning Framework\n";
    std::cout << "Usage: ./opengl_learning [demo_name]\n\n";
    std::cout << "Available demos:\n";
    std::cout << "  triangle  - Basic triangle rendering with color controls\n";
    std::cout << "  texture   - Texture mapping demo with filtering options\n";
    std::cout << "\nControls:\n";
    std::cout << "  ESC       - Exit application\n";
    std::cout << "  GUI       - Use ImGui interface for real-time parameter control\n";
    std::cout << "\nIf no demo is specified, you can select one from the GUI.\n";
}