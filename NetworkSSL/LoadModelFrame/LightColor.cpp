#include "LightColor.h"
#include "ResourceManager.h"
#include <glad/glad.h>
#include <imgui.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "stb_image.h"

void LightColorDemo::Initialize() {
    std::cout << "\n========== LightColorDemo with Texture Maps ==========" << std::endl;
    // 创建着色器
    phongShader = ResourceManager::LoadShader("phong",
        "shaders/lightcolor/phong.vs", "shaders/lightcolor/phong.fs");
    gouraudShader = ResourceManager::LoadShader("gouraud",
        "shaders/lightcolor/gouraud.vs", "shaders/lightcolor/gouraud.fs");
    lightCubeShader = ResourceManager::LoadShader("lightCube",
        "shaders/lightcolor/lightCube.vs", "shaders/lightcolor/lightCube.fs");

    setupGeometry();
    createTextures();

    // 设置相机
    camera->SetType(CameraType::ORBIT);
    camera->radius = 3.0f;
    camera->target = glm::vec3(0.0f, 0.0f, 0.0f);

    // 初始化网格和坐标轴
    gridAxisHelper->Initialize();
    // 可选：根据相机距离自动调整
    gridAxisHelper->UpdateGridForCamera(camera->radius);

    // 创建光源
    auto mainLight = lightManager->CreatePointLight(glm::vec3(1.2f, 1.0f, 2.0f));
    mainLight->ambient = glm::vec3(0.2f);
    mainLight->diffuse = glm::vec3(0.8f);
    mainLight->specular = glm::vec3(1.0f);
    mainLight->constant = 1.0f;
    mainLight->linear = 0.09f;
    mainLight->quadratic = 0.032f;

    auto dirLight = lightManager->CreateDirectionalLight();
    dirLight->direction = glm::vec3(-0.2f, -1.0f, -0.3f);
    dirLight->ambient = glm::vec3(0.05f);
    dirLight->diffuse = glm::vec3(0.2f);
    dirLight->specular = glm::vec3(0.3f);

    std::cout << "Texture mapping enabled:" << std::endl;
    std::cout << "  - Diffuse Map:  material.diffuse" << std::endl;
    std::cout << "  - Specular Map: material.specular" << std::endl;
    std::cout << "========== Initialize Complete ==========" << std::endl << std::endl;
}

void LightColorDemo::setupGeometry() {
    float vertices[] = {
        // positions          // normals           // texture coords
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f,  0.0f,

        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f,  1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f,  0.0f,

        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f,  0.0f,

        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f,  1.0f,

        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f,  0.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f,  1.0f
    };

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glGenVertexArrays(1, &lightVAO);
    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
}

void LightColorDemo::createTextures() {
    std::cout << "Loading textures..." << std::endl;

    // 加载漫反射贴图
    diffuseMap = loadTextureFromFile("container2.png");
    if (diffuseMap == 0) {
        diffuseMap = loadTextureFromFile("container.jpg");
    }
    if (diffuseMap == 0) {
        std::cout << "Failed to load diffuse map, creating fallback" << std::endl;
        diffuseMap = createFallbackTexture(255, 128, 64);  // 橙色
    }

    // 加载镜面光贴图
    specularMap = loadTextureFromFile("container2_specular.png");
    if (specularMap == 0) {
        std::cout << "Failed to load specular map, creating fallback" << std::endl;
        specularMap = createFallbackTexture(128, 128, 128);  // 灰色
    }

    // 设置着色器纹理单元
    phongShader->use();
    phongShader->setInt("material.diffuse", 0);   // 纹理单元0
    phongShader->setInt("material.specular", 1);  // 纹理单元1
    phongShader->setFloat("material.shininess", materialShininess);

    gouraudShader->use();
    gouraudShader->setInt("texture_diffuse", 0);
}

unsigned int LightColorDemo::loadTextureFromFile(const char* path) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrChannels;
    unsigned char* data = stbi_load(path, &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        std::cout << "Loaded texture: " << path << " (" << width << "x" << height << ")" << std::endl;
        return textureID;
    }
    else {
        std::cout << "Failed to load: " << path << std::endl;
        glDeleteTextures(1, &textureID);
        return 0;
    }
}

unsigned int LightColorDemo::createFallbackTexture(unsigned char r, unsigned char g, unsigned char b) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // 创建2x2的纹理
    unsigned char data[] = {
        r, g, b, 255,  r, g, b, 255,
        r, g, b, 255,  r, g, b, 255
    };

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return textureID;
}

void LightColorDemo::Update(float deltaTime) {
    // 更新逻辑
}

void LightColorDemo::Render() {
    glEnable(GL_DEPTH_TEST);

    RenderGridAndAxis();

    glm::mat4 view = GetViewMatrix();
    glm::mat4 projection = GetProjectionMatrix();

    // 模型变换
    glm::mat4 model = glm::mat4(1.0f);
    if (!auto_rotate_) {
        model = glm::rotate(model, glm::radians(model_angle_), glm::vec3(1.0f, 0.0f, 0.0f));
    }
    else {
        model = glm::rotate(model, rotate_speed_ * (float)glfwGetTime() * glm::radians(model_angle_),
            glm::vec3(0.5f, 1.0f, 0.0f));
    }

    // 渲染立方体
    if (use_gouraud_) {
        gouraudShader->use();
        const auto& lights = lightManager->GetLights();
        glm::vec3 lightPos(0.0f);
        for (const auto& light : lights) {
            if (light->type == LightType::POINT && light->enabled) {
                lightPos = light->position;
                break;
            }
        }
        gouraudShader->setVec3("lightPos", lightPos);
        gouraudShader->setVec3("lightColor", 1.0f, 1.0f, 1.0f);
        gouraudShader->setVec3("viewPos", camera->position);
        gouraudShader->setFloat("ambientStrength", 0.2f);
        gouraudShader->setFloat("specularStrength", 0.5f);
        gouraudShader->setMat4("model", model);
        gouraudShader->setMat4("view", view);
        gouraudShader->setMat4("projection", projection);

        // 绑定漫反射贴图
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
    }
    else {
        phongShader->use();
        phongShader->setMat4("model", model);
        phongShader->setMat4("view", view);
        phongShader->setMat4("projection", projection);
        phongShader->setVec3("viewPos", camera->position);
        phongShader->setFloat("material.shininess", materialShininess);

        // 绑定两个纹理
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap);

        // 应用光照
        lightManager->ApplyLightsToShader(*phongShader);
    }

    // 渲染立方体
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // 渲染光源立方体
    if (showLightCubes) {
        lightCubeShader->use();
        lightCubeShader->setMat4("view", view);
        lightCubeShader->setMat4("projection", projection);

        const auto& lights = lightManager->GetLights();
        for (const auto& light : lights) {
            if (!light->enabled) continue;

            if (light->type == LightType::POINT || light->type == LightType::SPOT) {
                glm::mat4 lightModel = glm::mat4(1.0f);
                lightModel = glm::translate(lightModel, light->position);
                lightModel = glm::scale(lightModel, glm::vec3(0.2f));
                lightCubeShader->setMat4("model", lightModel);
                lightCubeShader->setVec3("lightColor", light->diffuse);
                glBindVertexArray(lightVAO);
                glDrawArrays(GL_TRIANGLES, 0, 36);
            }
        }
    }

    glBindVertexArray(0);
}

void LightColorDemo::RenderImGui() {

    // ImGui::Begin("LightColor Demo with Texture Maps");

    ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Texture Maps:");
    ImGui::BulletText("Diffuse Map: Controls surface color");
    ImGui::BulletText("Specular Map: Controls reflective areas");
    ImGui::Text("Expected files:");
    ImGui::BulletText("container2.png or container.jpg");
    ImGui::BulletText("container2_specular.png");

    ImGui::Separator();
    ImGui::Text("Shading Model:");
    if (ImGui::RadioButton("Phong (Multi-texture)", !use_gouraud_)) {
        use_gouraud_ = false;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("Gouraud (Diffuse only)", use_gouraud_)) {
        use_gouraud_ = true;
    }

    ImGui::Separator();
    ImGui::Text("Material Properties:");
    ImGui::SliderFloat("Shininess", &materialShininess, 1.0f, 256.0f);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Controls size of specular highlights\nLower = larger, softer\nHigher = smaller, sharper");
    }

    ImGui::Separator();
    ImGui::Text("Model Transform:");
    ImGui::SliderFloat("Angle", &model_angle_, -180.0f, 180.0f);
    ImGui::Checkbox("Auto Rotate", &auto_rotate_);
    if (auto_rotate_) {
        ImGui::SliderFloat("Speed", &rotate_speed_, 0.1f, 5.0f);
    }

    ImGui::Separator();
    ImGui::Checkbox("Show Light Cubes", &showLightCubes);

    ImGui::Separator();
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "What you should see:");
    ImGui::BulletText("Container texture on cube faces");
    ImGui::BulletText("Bright specular highlights on metal rim");
    ImGui::BulletText("Dark areas on wooden parts (low specular)");
    ImGui::BulletText("Yellow light source cube");

    // ImGui::End();
}

void LightColorDemo::Cleanup() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &lightVAO);
    glDeleteBuffers(1, &VBO);
    glDeleteTextures(1, &diffuseMap);
    glDeleteTextures(1, &specularMap);
}

std::string LightColorDemo::GetName() const {
    return "LightColor with Texture Maps";
}
