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
    // GLFW ��ʼ��
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // ��������
    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Learning Framework v2.0", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // GLAD ��ʼ��
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    // ע�����ص�
    MouseManager::Get().RegisterCallbacks(window);

    // ImGui ��ʼ��
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // ������Ȳ���
    glEnable(GL_DEPTH_TEST);

    // ע�� Demos
    demoManager.RegisterDemo("Model Loading", std::make_unique<ModelLoadingDemo>());
    demoManager.RegisterDemo("LightColor Demo", std::make_unique<LightColorDemo>());

    std::cout << "OpenGL Learning Framework v2.0 initialized successfully!" << std::endl;
    std::cout << "OpenGL Version: " << glGetString(GL_VERSION) << std::endl;
    std::cout << "GLSL Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    return true;
}

void OpenGLApp::Run(const std::string& initialDemo) {
    // ���ó�ʼ Demo
    if (!initialDemo.empty()) {
        demoManager.SetCurrentDemo(initialDemo);
    }
    else {
        // Ĭ��ѡ���һ��
        auto names = demoManager.GetDemoNames();
        if (!names.empty()) {
            demoManager.SetCurrentDemo(names[0]);
        }
    }

    // ��ѭ��
    while (!glfwWindowShouldClose(window)) {
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window);

        // ����
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // ���º���Ⱦ
        demoManager.Update(window, deltaTime);
        demoManager.Render();

        // ImGui ��Ⱦ
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        demoManager.RenderImGui();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // �����������
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

    // ���������в���
    std::string initialDemo;
    if (argc > 1) {
        initialDemo = argv[1];
    }

    // ���û��ָ��demo��ָ����help����ʾ�÷���Ϣ
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