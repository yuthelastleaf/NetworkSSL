#include <glad/glad.h>
#include <imgui.h>
#include <iostream>
#include "LightColor.h"
#include <GLFW/glfw3.h>

#include "stb_image.h"
#include "MouseMng.h"

void LightColorDemo::Initialize() {
    // 着色器代码
    const char* vertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aNormal;\n"
        "layout (location = 2) in vec2 aTexCoord;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "out vec2 TexCoord;\n"
        "out vec3 Normal;\n"
        "out vec3 FragPos;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
        "   FragPos = vec3(model * vec4(aPos, 1.0));\n"
        "   TexCoord = vec2(aTexCoord.x, aTexCoord.y);\n"
        "   Normal = aNormal;\n"
        "}\0";

    const char* fragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec2 TexCoord;\n"
        "in vec3 Normal;\n"
        "in vec3 FragPos;\n"
        "uniform sampler2D texture;\n"
        "uniform vec3 lightColor;\n"
        "uniform vec3 lightPos;\n"
        "void main()\n"
        "{\n"
        "   FragColor = texture(texture, TexCoord) * vec4(lightColor, 1.0);\n"
        "   vec3 norm = normalize(Normal);\n"
        "   vec3 lightDir = normalize(lightPos - FragPos);\n"
        "}\0";

    shader = std::make_unique<Shader>(vertexShaderSource, fragmentShaderSource, true);

    createBasicTextures();

    // 创建各种颜色的纯色纹理
    whiteTexture = createSolidColorTexture(1.0f, 1.0f, 1.0f, 1.0f);
    redTexture = createSolidColorTexture(1.0f, 0.0f, 0.0f, 1.0f);
    greenTexture = createSolidColorTexture(0.0f, 1.0f, 0.0f, 1.0f);
    blueTexture = createSolidColorTexture(0.0f, 0.0f, 1.0f, 1.0f);

    //float vertices[] = {
    //    //     ---- 位置 ----       ---- 颜色 ----     - 纹理坐标 -
    //         0.5f,  0.5f, 0.0f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,   // 右上
    //         0.5f, -0.5f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,   // 右下
    //        -0.5f, -0.5f, 0.0f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,   // 左下
    //        -0.5f,  0.5f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f    // 左上
    //};

    float vertices[] = {
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f
    };
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // color attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // 初始化轨道相机
    updateOrbitCamera();
}

void LightColorDemo::Update(float deltaTime) {


}

void LightColorDemo::Render() {

    // 动态获取当前窗口大小
    int width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    // 计算动态宽高比
    float aspectRatio = (width > 0 && height > 0) ? (float)width / (float)height : 1.0f;

    //// 创建变换矩阵
    //glm::mat4 transform = glm::mat4(1.0f);
    //// 先应用纵横比修正
    //transform = glm::scale(transform, glm::vec3(1.0f / aspectRatio, 1.0f, 1.0f));

    shader->use();
    glEnable(GL_DEPTH_TEST);

    glm::mat4 model = glm::mat4(1.0f);
    if (!auto_rotate_) {
        model = glm::rotate(model, glm::radians(model_angle_), glm::vec3(1.0f, 0.0f, 0.0f));
    }
    else {
        model = glm::rotate(model, rotate_speed_ * (float)glfwGetTime() * glm::radians(model_angle_), glm::vec3(0.5f, 1.0f, 0.0f));
    }
    //glm::mat4 view = glm::mat4(1.0f);
    //// 注意，我们将矩阵向我们要进行移动场景的反方向移动。
    glm::mat4 view = view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);;

    glm::mat4 projection = glm::mat4(1.0f);
    projection = glm::perspective(glm::radians(fovy_), aspectRatio, 0.1f, 100.0f);

    shader->setMat4("model", model);
    shader->setMat4("view", view);
    shader->setMat4("projection", projection);
    shader->setVec3("lightColor", 1.0f, 1.0f, 1.0f); // 正常白光
    // 渲染代码
    // bind Texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture1);
    /*glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, texture2);*/

    // render container
    // ourShader.use();
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, whiteTexture);

    glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
    glm::mat4 light_model = glm::mat4(1.0f);
    light_model = glm::translate(light_model, lightPos);
    light_model = glm::scale(light_model, glm::vec3(0.2f)); // 缩小光源
    shader->setMat4("model", light_model);
    shader->setVec3("lightColor", 1.0f, 1.0f, 0.0f);

    glDrawArrays(GL_TRIANGLES, 0, 36);

    // glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void LightColorDemo::RenderImGui() {
    static float mixValue = 0.2f;
    static int wrapMode = 0;
    static bool useNearest = true;

    ImGui::Begin("Transform Demo Controls");

    // 轨道相机控制
    ImGui::Text("Orbit Camera Controls:");
    ImGui::Separator();

    ImGui::SliderFloat("Orbit Radius", &orbit_radius_, 1.0f, 10.0f);
    ImGui::SliderFloat("Orbit Yaw", &orbit_yaw_, -180.0f, 180.0f);
    ImGui::SliderFloat("Orbit Pitch", &orbit_pitch_, -89.0f, 89.0f);
    ImGui::SliderFloat3("Target Position", &orbit_target_.x, -5.0f, 5.0f);

    // 相机设置
    ImGui::Text("Camera Settings:");
    ImGui::SliderFloat("Mouse Sensitivity", &mouse_sens_, 10.0f, 200.0f);
    ImGui::SliderFloat("Scroll Sensitivity", &scroll_sensitivity_, 0.1f, 2.0f);
    ImGui::SliderFloat("Move Speed", &camera_speed_, 0.5f, 10.0f);

    // 变换控制
    ImGui::Separator();
    ImGui::Text("Model Controls:");

    ImGui::SliderFloat("Model Angle", &model_angle_, -180.0f, 180.0f);
    ImGui::SliderFloat("FOV", &fovy_, 1.0f, 90.0f);

    ImGui::Checkbox("Auto Rotate", &auto_rotate_);
    if (auto_rotate_) {
        ImGui::SliderFloat("Rotation Speed", &rotate_speed_, 0.1f, 5.0f);
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

void LightColorDemo::Cleanup() {
    // 清理资源
    shader.reset();
}

void LightColorDemo::ProcInput(GLFWwindow* window, float deltaTime)
{
    // 鼠标左键拖拽旋转
    if (MouseManager::Get().IsLeftButtonPressed()) {
        glm::vec2 mouse_delta = MouseManager::Get().GetMouseDelta();

        orbit_yaw_ += mouse_delta.x * mouse_sens_ * deltaTime;
        orbit_pitch_ -= mouse_delta.y * mouse_sens_ * deltaTime;  // 注意这里是减号，让鼠标上移视角上移
    }

    // 滚轮控制缩放
    float scroll_delta = MouseManager::Get().GetScrollY();
    if (abs(scroll_delta) > 0.001f) {
        orbit_radius_ -= scroll_delta * scroll_sensitivity_;
    }

    // WASD 键移动目标点（可选功能）
    float move_speed = camera_speed_ * deltaTime;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        orbit_target_.z -= move_speed;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        orbit_target_.z += move_speed;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        orbit_target_.x -= move_speed;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        orbit_target_.x += move_speed;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        orbit_target_.y -= move_speed;
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        orbit_target_.y += move_speed;

    // 更新相机位置
    updateOrbitCamera();
}

std::string LightColorDemo::GetName() const {
    return "Camera Demo";
}

void LightColorDemo::createBasicTextures()
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

    shader->use();
    // or set it via the texture class
    shader->setInt("texture", 0);

}

void LightColorDemo::applyTextureSettings(unsigned int textureID, int wrapMode, bool useNearest) {
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

unsigned int LightColorDemo::createSolidColorTexture(float r, float g, float b, float a) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // 将0-1的颜色值转换为0-255
    unsigned char colorData[] = {
        (unsigned char)(r * 255),
        (unsigned char)(g * 255),
        (unsigned char)(b * 255),
        (unsigned char)(a * 255)
    };

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, colorData);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    return textureID;
}

void LightColorDemo::updateOrbitCamera() {
    // 限制垂直角度，避免翻转
    if (orbit_pitch_ > 89.0f) orbit_pitch_ = 89.0f;
    if (orbit_pitch_ < -89.0f) orbit_pitch_ = -89.0f;

    // 限制距离
    if (orbit_radius_ < 1.0f) orbit_radius_ = 1.0f;
    if (orbit_radius_ > 10.0f) orbit_radius_ = 10.0f;

    // 根据球坐标计算相机位置
    float x = orbit_radius_ * cos(glm::radians(orbit_pitch_)) * cos(glm::radians(orbit_yaw_));
    float y = orbit_radius_ * sin(glm::radians(orbit_pitch_));
    float z = orbit_radius_ * cos(glm::radians(orbit_pitch_)) * sin(glm::radians(orbit_yaw_));

    cameraPos = orbit_target_ + glm::vec3(x, y, z);
    cameraFront = glm::normalize(orbit_target_ - cameraPos);
}
