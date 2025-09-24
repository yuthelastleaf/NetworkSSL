#include "demo.h"
#include "shader.h"
#include <glad/glad.h>
#include <imgui.h>
#include <iostream>

void TriangleDemo::Initialize() {
    // 顶点着色器源码
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
        "}\0";

    // 片段着色器源码
    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "uniform vec3 ourColor;\n"
        "void main()\n"
        "{\n"
        "   FragColor = vec4(ourColor, 1.0f);\n"
        "}\n\0";

    // 使用 Shader 类
    shader = std::make_unique<Shader>(vertexShaderSource, fragmentShaderSource, true);

    // 顶点数据
    float vertices[] = {
        -0.5f, -0.5f, 0.0f,
         0.5f, -0.5f, 0.0f,
         0.0f,  0.5f, 0.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void TriangleDemo::Update(float deltaTime) {}

void TriangleDemo::Render() {
    shader->use();
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void TriangleDemo::RenderImGui() {
    static float color[3] = { 1.0f, 0.5f, 0.2f };

    ImGui::Begin("Triangle Demo Controls");
    ImGui::ColorEdit3("Triangle Color", color);
    ImGui::Text("This is a basic triangle demo");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
        1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();

    // 设置 uniform
    shader->use();
    shader->setVec3("ourColor", color[0], color[1], color[2]);
}

void TriangleDemo::Cleanup() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    shader.reset();
}

std::string TriangleDemo::GetName() const {
    return "Triangle Demo";
}