// ========================================
// main.cpp - 主程序入口
// ========================================
#include "demo.h"
#include "utils.h"
#include <iostream>

int main(int argc, char* argv[]) {
    std::string selectedDemo = "";

    // 命令行参数处理
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--help" || arg == "-h") {
            printUsage();
            return 0;
        }
        selectedDemo = arg;
    }

    // 创建应用
    OpenGLApp app;

    if (!app.Initialize()) {
        return -1;
    }

    std::cout << "OpenGL Learning Framework Started!" << std::endl;
    std::cout << "==================================" << std::endl;
    if (!selectedDemo.empty()) {
        std::cout << "Starting with demo: " << selectedDemo << std::endl;
    }
    else {
        std::cout << "Use the GUI to select a demo" << std::endl;
    }
    std::cout << "Press ESC to exit, use ImGui for controls" << std::endl;

    app.Run(selectedDemo);
    app.Cleanup();

    return 0;
}