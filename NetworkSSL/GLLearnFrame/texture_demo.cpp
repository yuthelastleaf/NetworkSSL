#include "demo.h"
#include "shader.h"
#include <glad/glad.h>
#include <imgui.h>
#include <iostream>

void TextureDemo::Initialize() {
    // 着色器代码
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec2 aTexCoord;\n"
        "out vec2 TexCoord;\n"
        "void main() {\n"
        "   gl_Position = vec4(aPos, 1.0);\n"
        "   TexCoord = aTexCoord;\n"
        "}\0";

    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec2 TexCoord;\n"
        "uniform sampler2D texture1;\n"
        "uniform sampler2D texture2;\n"
        "uniform float mixValue;\n"
        "void main() {\n"
        "   FragColor = mix(texture(texture1, TexCoord), \n"
        "                   texture(texture2, vec2(1.0 - TexCoord.x, TexCoord.y)), \n"
        "                   mixValue);\n"
        "}\n\0";

    // 创建着色器
    shader = std::make_unique<Shader>(vertexShaderSource, fragmentShaderSource, true);

    // 顶点数据
    float vertices[] = {
        // positions          // texture coords
         0.5f,  0.5f, 0.0f,   1.0f, 1.0f,
         0.5f, -0.5f, 0.0f,   1.0f, 0.0f,
        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f,
        -0.5f,  0.5f, 0.0f,   0.0f, 1.0f
    };

    unsigned int indices[] = {
        0, 1, 3,
        1, 2, 3
    };

    // 设置缓冲区
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 纹理坐标属性
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // 创建基础纹理
    createBasicTextures();

    // 设置纹理单元
    shader->use();
    shader->setInt("texture1", 0);
    shader->setInt("texture2", 1);
}

void TextureDemo::createBasicTextures() {
    // 创建棋盘纹理
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);

    unsigned char checkerboard[64 * 64 * 3];
    for (int i = 0; i < 64 * 64; i++) {
        int x = i % 64;
        int y = i / 64;
        bool isWhite = ((x / 8) + (y / 8)) % 2;
        checkerboard[i * 3] = isWhite ? 255 : 0;
        checkerboard[i * 3 + 1] = isWhite ? 255 : 0;
        checkerboard[i * 3 + 2] = isWhite ? 255 : 0;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, checkerboard);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 创建渐变纹理
    glGenTextures(1, &texture2);
    glBindTexture(GL_TEXTURE_2D, texture2);

    unsigned char gradient[64 * 64 * 3];
    for (int i = 0; i < 64 * 64; i++) {
        int x = i % 64;
        int y = i / 64;
        gradient[i * 3] = x * 4;     // Red gradient
        gradient[i * 3 + 1] = y * 4;   // Green gradient
        gradient[i * 3 + 2] = 128;     // Blue constant
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, gradient);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    std::cout << "Basic textures created successfully!" << std::endl;
}

void TextureDemo::Update(float deltaTime) {}

void TextureDemo::Render() {
    // 绑定纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture2);

    shader->use();
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void TextureDemo::RenderImGui() {
    static float mixValue = 0.2f;
    static int wrapMode = 0;
    static bool useNearest = false;

    ImGui::Begin("Texture Demo Controls");

    if (ImGui::SliderFloat("Mix Value", &mixValue, 0.0f, 1.0f)) {
        shader->use();
        shader->setFloat("mixValue", mixValue);
    }

    const char* wrapModes[] = { "REPEAT", "MIRRORED_REPEAT", "CLAMP_TO_EDGE", "CLAMP_TO_BORDER" };
    ImGui::Combo("Wrap Mode", &wrapMode, wrapModes, 4);
    ImGui::Checkbox("Use GL_NEAREST", &useNearest);

    if (ImGui::Button("Apply Texture Settings")) {
        applyTextureSettings(texture2, wrapMode, useNearest);
    }

    ImGui::Text("Checkerboard pattern (Texture 1)");
    ImGui::Text("Color gradient (Texture 2)");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
        1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
}

void TextureDemo::applyTextureSettings(unsigned int textureID, int wrapMode, bool useNearest) {
    glBindTexture(GL_TEXTURE_2D, textureID);

    GLenum wrap;
    switch (wrapMode) {
    case 0: wrap = GL_REPEAT; break;
    case 1: wrap = GL_MIRRORED_REPEAT; break;
    case 2: wrap = GL_CLAMP_TO_EDGE; break;
    case 3: wrap = GL_CLAMP_TO_BORDER; break;
    default: wrap = GL_REPEAT; break;
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);

    if (wrap == GL_CLAMP_TO_BORDER) {
        float borderColor[] = { 1.0f, 1.0f, 0.0f, 1.0f };
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    }

    GLenum filter = useNearest ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
}

void TextureDemo::Cleanup() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteTextures(1, &texture1);
    glDeleteTextures(1, &texture2);
    shader.reset();
}

std::string TextureDemo::GetName() const {
    return "Texture Demo";
}