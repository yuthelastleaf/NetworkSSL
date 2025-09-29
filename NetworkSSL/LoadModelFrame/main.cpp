#include "demo.h"
#include "utils.h"
#include "ResourceManager.h"
#include "MouseMng.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>

#include <Windows.h>

#include "LightColor.h"

bool OpenGLApp::Initialize() {
    // GLFW 初始化
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 创建窗口
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Learning Framework v2.0", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // GLAD 初始化
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    // 注册鼠标回调
    MouseManager::Get().RegisterCallbacks(window);

    // ImGui 初始化
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // 启用深度测试
    glEnable(GL_DEPTH_TEST);

    // 注册 Demos
    demoManager.RegisterDemo("Model Loading", std::make_unique<ModelLoadingDemo>());
    demoManager.RegisterDemo("LightColor Demo", std::make_unique<LightColorDemo>());

    std::cout << "OpenGL Learning Framework v2.0 initialized successfully!" << std::endl;
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    return true;
}

void OpenGLApp::Run(const std::string& initialDemo) {
    // 设置初始 Demo
    if (!initialDemo.empty()) {
        demoManager.SetCurrentDemo(initialDemo);
    }
    else {
        // 默认选择第一个
        auto names = demoManager.GetDemoNames();
        if (!names.empty()) {
            demoManager.SetCurrentDemo(names[0]);
        }
    }

    // 主循环
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // 清屏
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 更新和渲染
        demoManager.Update(window, deltaTime);
        demoManager.Render();

        // ImGui 渲染
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        demoManager.RenderImGui();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // 重置鼠标增量
        MouseManager::Get().ResetDeltas();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void OpenGLApp::Cleanup() {
    demoManager.Cleanup();
    ResourceManager::Clear();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
}

int main(int argc, char* argv[]) {
    OpenGLApp app;

    if (!app.Initialize()) {
        std::cerr << "Failed to initialize OpenGL application" << std::endl;
        return -1;
    }

    // 解析命令行参数
    std::string initialDemo;
    if (argc > 1) {
        initialDemo = argv[1];
    }

    // 如果没有指定demo或指定了help，显示用法信息
    if (argc == 1 || (argc > 1 && (std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h"))) {
        printUsage();
        if (argc > 1) {
            app.Cleanup();
            return 0;
        }
    }

    try {
        app.Run(initialDemo);
    }
    catch (const std::exception& e) {
        std::cerr << "Application error: " << e.what() << std::endl;
        app.Cleanup();
        return -1;
    }

    app.Cleanup();
    return 0;
}