#include "GLDraw.h"
#include <iostream>

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

int main()
{
    

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    //glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    // ImGui::StyleColorsClassic();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    glViewport(0, 0, 800, 600);

    
    GLDraw draw;
    // draw.InitTrangle();
    // draw.InitRectangle();
    // draw.InitNearTrangle();
    // draw.InitTwoTrangle();
    // draw.InitUniformDemo();
    // draw.InitMultiProp();
    // draw.InitMultiPropMove();
    // draw.InitMultiPropLRMove();
    // draw.InitVecToClr();
    // draw.InitTexture();
    draw.InitFaceTexture();

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        // draw.DrawTwo();
        // draw.DrawUniform();
        // draw.DrawMultiMove();
        // draw.DrawMultiLRMove();
        // draw.DrawVecToClr();
        // draw.DrawTexture();
        draw.DrawTwoTexture();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    // Cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwTerminate();
    return 0;
}
