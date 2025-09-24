#include "demo.h"
#include "utils.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <memory>
#include "Transform.h"
#include "in3d.h"

bool OpenGLApp::Initialize() {
    // GLFW 初始化
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 创建窗口
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Learning Framework", NULL, NULL);
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

    // ImGui 初始化
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // 注册 Demos
    demoManager.RegisterDemo("triangle", std::make_unique<TriangleDemo>());
    demoManager.RegisterDemo("texture", std::make_unique<TextureDemo>());
    demoManager.RegisterDemo("transform", std::make_unique<TransformDemo>());
    demoManager.RegisterDemo("in3d", std::make_unique<in3dDemo>());

    std::cout << "OpenGL Learning Framework initialized successfully!" << std::endl;
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
        float currentFrame = glfwGetTime();
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // 清屏
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        // glClear(GL_COLOR_BUFFER_BIT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // 更新和渲染
        demoManager.Update(deltaTime);
        demoManager.Render();

        // ImGui 渲染
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        demoManager.RenderImGui();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void OpenGLApp::Cleanup() {
    demoManager.Cleanup();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
}
