#include <glad/glad.h>
#include <imgui.h>
#include <iostream>
#include "Transform.h"
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

void TransformDemo::Initialize() {
    // 着色器代码
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aColor;\n"
        "layout (location = 2) in vec2 aTexCoord;\n"
        "out vec3 ourColor;\n"
        "out vec2 TexCoord;\n"
        "uniform mat4 transform;\n"  // 添加变换矩阵
        "void main()\n"
        "{\n"
        "   gl_Position = transform * vec4(aPos, 1.0);\n"
        "   ourColor = aColor;\n"
        "   TexCoord = vec2(aTexCoord.x, aTexCoord.y);\n"
        "}\0";

    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec3 ourColor;\n"
        "in vec2 TexCoord;\n"
        "uniform sampler2D texture1;\n"
        "uniform sampler2D texture2;\n"
        "uniform float mixValue; // 新增\n"
        "void main()\n"
        "{\n"
        // "   FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), 0.2) * vec4(ourColor, 1.0);\n"
        "   FragColor = mix(texture(texture1, TexCoord), texture(texture2, vec2(1.0 - TexCoord.x, TexCoord.y)), mixValue) * vec4(ourColor, 1.0);\n"
        "}\0";

    shader = std::make_unique<Shader>(vertexShaderSource, fragmentShaderSource, true);

    createBasicTextures();

    float vertices[] = {
        //     ---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -
             0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // 右上
             0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // 右下
            -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // 左下
            -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // 左上
    };

    unsigned int indices[] = {
    0, 1, 3, // first triangle
    1, 2, 3  // second triangle
    };
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // 你的顶点数据和缓冲区设置
    // ...
}

void TransformDemo::Update(float deltaTime) {
    // 自动旋转逻辑
    if (autoRotate) {
        rotationAngle += rotationSpeed * deltaTime;
        if (rotationAngle > 360.0f) {
            rotationAngle -= 360.0f;
        }
    }
}

void TransformDemo::Render() {

    float aspectRatio = 1200.0f / 800.0f;
    // 创建变换矩阵
    glm::mat4 transform = glm::mat4(1.0f);

    // 先应用纵横比修正
    transform = glm::scale(transform, glm::vec3(1.0f / aspectRatio, 1.0f, 1.0f));


    // 应用变换（顺序很重要！）
    transform = glm::translate(transform, glm::vec3(translateX, translateY, 0.0f));
    transform = glm::rotate(transform, glm::radians(rotationAngle), glm::vec3(0.0f, 0.0f, 1.0f));
    transform = glm::scale(transform, glm::vec3(scaleX, scaleY, 1.0f));

    shader->use();
    shader->setMat4("transform", transform);
    // 渲染代码
    // bind Texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture2);

    // render container
    // ourShader.use();
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    // 第二个箱子 - 左上角自动缩放
    glm::mat4 transform2 = glm::mat4(1.0f);

    // 摆放在左上角 (NDC坐标系中左上角大约是 (-0.7, 0.7))
    transform2 = glm::translate(transform2, glm::vec3(-0.7f, 0.7f, 0.0f));

    // 使用 sin 函数实现缩放动画，避免负值
    float time = glfwGetTime();
    float scaleValue = 0.5f + 0.3f * sin(time);  // 范围：0.2 - 0.8
    // 或者使用绝对值确保不为负：float scaleValue = abs(sin(time)) * 0.5f + 0.3f;
    transform2 = glm::scale(transform2, glm::vec3(scaleValue, scaleValue, 1.0f));

    shader->setMat4("transform", transform2);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void TransformDemo::RenderImGui() {
    static float mixValue = 0.2f;
    static int wrapMode = 0;
    static bool useNearest = true;

    ImGui::Begin("Transform Demo Controls");

    // 变换控制
    ImGui::Text("Transform Controls:");
    ImGui::Separator();

    // 旋转控制
    ImGui::SliderFloat("Rotation (degrees)", &rotationAngle, 0.0f, 360.0f);
    ImGui::Checkbox("Auto Rotate", &autoRotate);
    if (autoRotate) {
        ImGui::SliderFloat("Rotation Speed", &rotationSpeed, 0.1f, 5.0f);
    }

    // 缩放控制
    ImGui::SliderFloat("Scale X", &scaleX, 0.1f, 3.0f);
    ImGui::SliderFloat("Scale Y", &scaleY, 0.1f, 3.0f);
    if (ImGui::Button("Reset Scale")) {
        scaleX = scaleY = 1.0f;
    }

    // 平移控制
    ImGui::SliderFloat("Translate X", &translateX, -1.0f, 1.0f);
    ImGui::SliderFloat("Translate Y", &translateY, -1.0f, 1.0f);
    if (ImGui::Button("Reset Position")) {
        translateX = translateY = 0.0f;
    }

    // 重置所有变换
    if (ImGui::Button("Reset All Transforms")) {
        rotationAngle = 0.0f;
        scaleX = scaleY = 1.0f;
        translateX = translateY = 0.0f;
        autoRotate = false;
    }

    ImGui::Separator();
    ImGui::Text("Texture Controls:");

    // 原有的纹理控制
    ImGui::SliderFloat("Mix Value", &mixValue, 0.0f, 1.0f);
    const char* wrapModes[] = { "REPEAT", "MIRRORED_REPEAT", "CLAMP_TO_EDGE", "CLAMP_TO_BORDER" };
    ImGui::Combo("Wrap Mode", &wrapMode, wrapModes, 4);
    ImGui::Checkbox("Use GL_NEAREST", &useNearest);

    // 应用设置
    applyTextureSettings(texture2, wrapMode, useNearest);
    shader->use();
    shader->setFloat("mixValue", mixValue);

    ImGui::End();
}

void TransformDemo::Cleanup() {
    // 清理资源
    shader.reset();
}

std::string TransformDemo::GetName() const {
    return "My New Demo";
}

void TransformDemo::createBasicTextures()
{
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int width, height, nrChannels;
    // 需要先下载container.jpg哦，地址是 https://learnopengl-cn.github.io/img/01/06/container.jpg 
    // 另外一个笑脸下载地址：https://learnopengl-cn.github.io/img/01/06/awesomeface.png
    unsigned char* data = stbi_load("container.jpg", &width, &height, &nrChannels, 0);

    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture1" << std::endl;
    }
    stbi_image_free(data);

    glGenTextures(1, &texture2);
    glBindTexture(GL_TEXTURE_2D, texture2);

    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    // glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    stbi_set_flip_vertically_on_load(true);
    data = stbi_load("awesomeface.png", &width, &height, &nrChannels, 0);

    if (data)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture2" << std::endl;
    }
    stbi_image_free(data);

    shader->use();
    // or set it via the texture class
    shader->setInt("texture1", 0);
    shader->setInt("texture2", 1);

}

void TransformDemo::applyTextureSettings(unsigned int textureID, int wrapMode, bool useNearest) {
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

    GLenum filter = useNearest ? GL_NEAREST : GL_LINEAR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
}
