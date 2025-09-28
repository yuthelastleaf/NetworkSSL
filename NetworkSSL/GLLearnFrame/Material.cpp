#include <glad/glad.h>
#include <imgui.h>
#include <iostream>
#include "Material.h"
#include <GLFW/glfw3.h>

#include "stb_image.h"
#include "MouseMng.h"

void MaterialDemo::Initialize() {
    // 标准着色器代码
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
        "   Normal = mat3(transpose(inverse(model))) * aNormal;\n"
        "}\0";

    // Phong着色片段着色器（材质参数）
    const char* fragmentShaderSource = "#version 330 core\n"
        "struct Material {\n"
        "    vec3 ambient;\n"
        "    vec3 diffuse;\n"
        "    vec3 specular;\n"
        "    float shininess;\n"
        "};\n"
        "struct Light {\n"
        "    vec3 position;\n"
        "    vec3 ambient;\n"
        "    vec3 diffuse;\n"
        "    vec3 specular;\n"
        "};\n"
        "out vec4 FragColor;\n"
        "in vec2 TexCoord;\n"
        "in vec3 Normal;\n"
        "in vec3 FragPos;\n"
        "uniform sampler2D texture1;\n"
        "uniform vec3 lightColor;\n"
        "uniform vec3 lightPos;\n"
        "uniform vec3 viewPos;\n"
        "uniform float ambientstrength;\n"
        "uniform float specularstrength;\n"
        "uniform Material material;\n"
        "uniform Light light;\n"
        "void main()\n"
        "{\n"
        "   vec3 norm = normalize(Normal);\n"
        "   vec3 lightDir = normalize(lightPos - FragPos);\n"
        "   vec3 viewDir = normalize(viewPos - FragPos);\n"
        "   vec3 reflectDir = reflect(-lightDir, norm);\n"
        "   float diff = max(dot(norm, lightDir), 0.0);\n"
        "   vec3 ambient = light.ambient * ambientstrength * lightColor * material.ambient;\n"
        "   vec3 diffuse = light.diffuse * (diff * material.diffuse) * lightColor;\n"
        "   float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);\n"
        "   vec3 specular = specularstrength * light.specular * spec * material.specular * lightColor;\n"
        "   FragColor = texture(texture1, TexCoord) * vec4(ambient + diffuse + specular, 1.0);\n"
        "}\0";

    // 贴图材质片段着色器
    const char* textureFragmentShaderSource = "#version 330 core\n"
        "struct Material {\n"
        "    sampler2D diffuse;\n"
        "    sampler2D specular;\n"
        "    float shininess;\n"
        "};\n"
        "struct Light {\n"
        "    vec3 position;\n"
        "    vec3 ambient;\n"
        "    vec3 diffuse;\n"
        "    vec3 specular;\n"
        "};\n"
        "out vec4 FragColor;\n"
        "in vec2 TexCoord;\n"
        "in vec3 Normal;\n"
        "in vec3 FragPos;\n"
        "uniform vec3 lightPos;\n"
        "uniform vec3 viewPos;\n"
        "uniform Material material;\n"
        "uniform Light light;\n"
        "void main()\n"
        "{\n"
        "    vec3 diffuseColor = texture(material.diffuse, TexCoord).rgb;\n"
        "    vec3 specularColor = texture(material.specular, TexCoord).rgb;\n"
        "    \n"
        "    vec3 norm = normalize(Normal);\n"
        "    vec3 lightDir = normalize(lightPos - FragPos);\n"
        "    vec3 viewDir = normalize(viewPos - FragPos);\n"
        "    vec3 reflectDir = reflect(-lightDir, norm);\n"
        "    \n"
        "    vec3 ambient = light.ambient * diffuseColor;\n"
        "    \n"
        "    float diff = max(dot(norm, lightDir), 0.0);\n"
        "    vec3 diffuse = light.diffuse * diff * diffuseColor;\n"
        "    \n"
        "    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);\n"
        "    vec3 specular = light.specular * spec * specularColor;\n"
        "    \n"
        "    vec3 result = ambient + diffuse + specular;\n"
        "    FragColor = vec4(result, 1.0);\n"
        "}\0";

    // 光源着色器
    const char* vlightShader = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
        "}\0";

    const char* flightShader = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "uniform vec3 lightColor;\n"
        "void main()\n"
        "{\n"
        "   FragColor = vec4(lightColor, 1.0);\n"
        "}\0";

    // Gouraud着色器
    const char* gouraudVertexShaderSource = "#version 330 core\n"
        "layout (location = 0) in vec3 aPos;\n"
        "layout (location = 1) in vec3 aNormal;\n"
        "layout (location = 2) in vec2 aTexCoord;\n"
        "uniform mat4 model;\n"
        "uniform mat4 view;\n"
        "uniform mat4 projection;\n"
        "uniform vec3 lightPos;\n"
        "uniform vec3 lightColor;\n"
        "uniform vec3 viewPos;\n"
        "uniform float ambientstrength;\n"
        "uniform float specularstrength;\n"
        "out vec2 TexCoord;\n"
        "out vec3 LightingColor;\n"
        "void main()\n"
        "{\n"
        "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
        "   vec3 FragPos = vec3(model * vec4(aPos, 1.0));\n"
        "   vec3 Normal = mat3(transpose(inverse(model))) * aNormal;\n"
        "   TexCoord = aTexCoord;\n"
        "   \n"
        "   vec3 norm = normalize(Normal);\n"
        "   vec3 lightDir = normalize(lightPos - FragPos);\n"
        "   vec3 viewDir = normalize(viewPos - FragPos);\n"
        "   vec3 reflectDir = reflect(-lightDir, norm);\n"
        "   \n"
        "   vec3 ambient = ambientstrength * lightColor;\n"
        "   \n"
        "   float diff = max(dot(norm, lightDir), 0.0);\n"
        "   vec3 diffuse = diff * lightColor;\n"
        "   \n"
        "   float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);\n"
        "   vec3 specular = specularstrength * spec * lightColor;\n"
        "   \n"
        "   LightingColor = ambient + diffuse + specular;\n"
        "}\0";

    const char* gouraudFragmentShaderSource = "#version 330 core\n"
        "out vec4 FragColor;\n"
        "in vec2 TexCoord;\n"
        "in vec3 LightingColor;\n"
        "uniform sampler2D texture1;\n"
        "void main()\n"
        "{\n"
        "   FragColor = texture(texture1, TexCoord) * vec4(LightingColor, 1.0);\n"
        "}\0";

    // 创建着色器
    shader = std::make_unique<Shader>(vertexShaderSource, fragmentShaderSource, true);
    texture_shader = std::make_unique<Shader>(vertexShaderSource, textureFragmentShaderSource, true);
    light_shader = std::make_unique<Shader>(vlightShader, flightShader, true);
    glight_shader = std::make_unique<Shader>(gouraudVertexShaderSource, gouraudFragmentShaderSource, true);

    createBasicTextures();
    createMaterialTextures();

    // 创建各种颜色的纯色纹理
    whiteTexture = createSolidColorTexture(1.0f, 1.0f, 1.0f, 1.0f);
    redTexture = createSolidColorTexture(1.0f, 0.0f, 0.0f, 1.0f);
    greenTexture = createSolidColorTexture(0.0f, 1.0f, 0.0f, 1.0f);
    blueTexture = createSolidColorTexture(0.0f, 0.0f, 1.0f, 1.0f);

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

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glGenVertexArrays(1, &lightVAO);
    glGenBuffers(1, &lightVBO);
    glBindVertexArray(lightVAO);
    glBindBuffer(GL_ARRAY_BUFFER, lightVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // 初始化轨道相机
    updateOrbitCamera();
}

void MaterialDemo::Update(float deltaTime) {
}

void MaterialDemo::Render() {
    // 动态获取当前窗口大小
    int width, height;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &width, &height);
    float aspectRatio = (width > 0 && height > 0) ? (float)width / (float)height : 1.0f;

    // 选择使用哪个着色器
    Shader* currentShader;
    if (use_gouraud_) {
        currentShader = glight_shader.get();
    }
    else if (use_texture_material_) {
        currentShader = texture_shader.get();
    }
    else {
        currentShader = shader.get();
    }

    currentShader->use();
    glEnable(GL_DEPTH_TEST);

    glm::mat4 model = glm::mat4(1.0f);
    if (!auto_rotate_) {
        model = glm::rotate(model, glm::radians(model_angle_), glm::vec3(1.0f, 0.0f, 0.0f));
    }
    else {
        model = glm::rotate(model, rotate_speed_ * (float)glfwGetTime() * glm::radians(model_angle_), glm::vec3(0.5f, 1.0f, 0.0f));
    }

    glm::mat4 view = glm::lookAt(cameraPos, orbit_target_, cameraUp);
    glm::mat4 projection = glm::perspective(glm::radians(fovy_), aspectRatio, 0.1f, 100.0f);

    // 设置uniform
    glm::vec3 lightPos(1.2f, 1.0f, 2.0f);
    currentShader->setVec3("lightPos", lightPos.x, lightPos.y, lightPos.z);
    currentShader->setVec3("viewPos", cameraPos.x, cameraPos.y, cameraPos.z);
    currentShader->setMat4("model", model);
    currentShader->setMat4("view", view);
    currentShader->setMat4("projection", projection);

    // 让light_color_影响所有光照分量
    glm::vec3 finalAmbient = light_ambient_ * light_color_;
    glm::vec3 finalDiffuse = light_diffuse_ * light_color_;
    glm::vec3 finalSpecular = light_specular_ * light_color_;

    if (use_texture_material_) {
        // 贴图材质模式
        currentShader->setFloat("material.shininess", material_shininess_);
        currentShader->setVec3("light.ambient", finalAmbient);
        currentShader->setVec3("light.diffuse", finalDiffuse);
        currentShader->setVec3("light.specular", finalSpecular);

        // 绑定贴图
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, diffuseMap_);
        currentShader->setInt("material.diffuse", 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, specularMap_);
        currentShader->setInt("material.specular", 1);
    }
    else if (!use_gouraud_) {
        // Phong着色材质参数模式
        currentShader->setVec3("lightColor", light_color_);
        currentShader->setFloat("ambientstrength", ambient_strength_);
        currentShader->setFloat("specularstrength", specular_strength_);

        currentShader->setVec3("material.ambient", material_ambient_);
        currentShader->setVec3("material.diffuse", material_diffuse_);
        currentShader->setVec3("material.specular", material_specular_);
        currentShader->setFloat("material.shininess", material_shininess_);

        currentShader->setVec3("light.ambient", finalAmbient);
        currentShader->setVec3("light.diffuse", finalDiffuse);
        currentShader->setVec3("light.specular", finalSpecular);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
    }
    else {
        // Gouraud着色模式
        currentShader->setVec3("lightColor", light_color_);
        currentShader->setFloat("ambientstrength", ambient_strength_);
        currentShader->setFloat("specularstrength", specular_strength_);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture1);
    }

    // 渲染立方体
    glBindVertexArray(VAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);

    // 渲染光源立方体
    light_shader->use();
    glm::mat4 light_model = glm::mat4(1.0f);
    light_model = glm::translate(light_model, lightPos);
    light_model = glm::scale(light_model, glm::vec3(0.2f));
    light_shader->setMat4("model", light_model);
    light_shader->setMat4("view", view);
    light_shader->setMat4("projection", projection);
    light_shader->setVec3("lightColor", 1.0f, 1.0f, 0.0f);
    glBindVertexArray(lightVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
}

void MaterialDemo::RenderImGui() {
    static float mixValue = 0.2f;
    static int wrapMode = 0;
    static bool useNearest = true;

    ImGui::Begin("Material Demo Controls");

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

    ImGui::Separator();

    // 着色模型选择
    ImGui::Text("Shading Model:");
    if (ImGui::RadioButton("Phong Shading", !use_gouraud_ && !use_texture_material_)) {
        use_gouraud_ = false;
        use_texture_material_ = false;
    }
    if (ImGui::RadioButton("Gouraud Shading", use_gouraud_)) {
        use_gouraud_ = true;
        use_texture_material_ = false;
    }
    if (ImGui::RadioButton("Texture Material", use_texture_material_)) {
        use_gouraud_ = false;
        use_texture_material_ = true;
    }

    ImGui::Separator();

    // 光照设置
    ImGui::Text("Light Settings:");
    if (!use_texture_material_) {
        ImGui::SliderFloat("Ambient Strength", &ambient_strength_, 0.0f, 1.0f);
        ImGui::SliderFloat("Specular Strength", &specular_strength_, 0.0f, 2.0f);
    }

    // 材质设置（仅在非贴图模式下显示）
    if (!use_texture_material_) {
        ImGui::Text("Material Properties:");
        ImGui::ColorEdit3("Material Ambient", &material_ambient_.x);
        ImGui::ColorEdit3("Material Diffuse", &material_diffuse_.x);
        ImGui::ColorEdit3("Material Specular", &material_specular_.x);
    }

    // shininess在所有模式下都显示
    ImGui::SliderFloat("Material Shininess", &material_shininess_, 1.0f, 256.0f);

    // 材质预设
    ImGui::Text("Material Presets:");
    if (ImGui::Button("Gold")) {
        material_ambient_ = glm::vec3(0.24725f, 0.1995f, 0.0745f);
        material_diffuse_ = glm::vec3(0.75164f, 0.60648f, 0.22648f);
        material_specular_ = glm::vec3(0.628281f, 0.555802f, 0.366065f);
        material_shininess_ = 51.2f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Silver")) {
        material_ambient_ = glm::vec3(0.19225f, 0.19225f, 0.19225f);
        material_diffuse_ = glm::vec3(0.50754f, 0.50754f, 0.50754f);
        material_specular_ = glm::vec3(0.508273f, 0.508273f, 0.508273f);
        material_shininess_ = 51.2f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Emerald")) {
        material_ambient_ = glm::vec3(0.0215f, 0.1745f, 0.0215f);
        material_diffuse_ = glm::vec3(0.07568f, 0.61424f, 0.07568f);
        material_specular_ = glm::vec3(0.633f, 0.727811f, 0.633f);
        material_shininess_ = 76.8f;
    }
    if (ImGui::Button("Red Plastic")) {
        material_ambient_ = glm::vec3(0.0f, 0.0f, 0.0f);
        material_diffuse_ = glm::vec3(0.5f, 0.0f, 0.0f);
        material_specular_ = glm::vec3(0.7f, 0.6f, 0.6f);
        material_shininess_ = 32.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Rubber")) {
        material_ambient_ = glm::vec3(0.02f, 0.02f, 0.02f);
        material_diffuse_ = glm::vec3(0.01f, 0.01f, 0.01f);
        material_specular_ = glm::vec3(0.4f, 0.4f, 0.4f);
        material_shininess_ = 10.0f;
    }

    ImGui::Separator();

    // 光源设置
    ImGui::Text("Light Properties:");
    ImGui::ColorEdit3("Light Color", &light_color_.x);
    ImGui::ColorEdit3("Light Ambient", &light_ambient_.x);
    ImGui::ColorEdit3("Light Diffuse", &light_diffuse_.x);
    ImGui::ColorEdit3("Light Specular", &light_specular_.x);

    // 光源预设
    ImGui::Text("Light Presets:");
    if (ImGui::Button("Daylight")) {
        light_color_ = glm::vec3(1.0f, 1.0f, 0.9f);
        light_ambient_ = glm::vec3(0.3f, 0.3f, 0.3f);
        light_diffuse_ = glm::vec3(0.8f, 0.8f, 0.7f);
        light_specular_ = glm::vec3(1.0f, 1.0f, 0.9f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Sunset")) {
        light_color_ = glm::vec3(1.0f, 0.6f, 0.3f);
        light_ambient_ = glm::vec3(0.2f, 0.1f, 0.05f);
        light_diffuse_ = glm::vec3(0.8f, 0.5f, 0.2f);
        light_specular_ = glm::vec3(1.0f, 0.6f, 0.3f);
    }
    ImGui::SameLine();
    if (ImGui::Button("Cool White")) {
        light_color_ = glm::vec3(0.9f, 0.95f, 1.0f);
        light_ambient_ = glm::vec3(0.2f, 0.2f, 0.3f);
        light_diffuse_ = glm::vec3(0.7f, 0.8f, 1.0f);
        light_specular_ = glm::vec3(0.9f, 0.95f, 1.0f);
    }

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

    ImGui::End();
}

void MaterialDemo::Cleanup() {
    shader.reset();
    texture_shader.reset();
    light_shader.reset();
    glight_shader.reset();
}

void MaterialDemo::ProcInput(GLFWwindow* window, float deltaTime) {
    // 鼠标左键拖拽旋转
    if (MouseManager::Get().IsLeftButtonPressed()) {
        glm::vec2 mouse_delta = MouseManager::Get().GetMouseDelta();

        orbit_yaw_ += mouse_delta.x * mouse_sens_ * deltaTime;
        orbit_pitch_ -= mouse_delta.y * mouse_sens_ * deltaTime;
    }

    // 滚轮控制缩放
    float scroll_delta = MouseManager::Get().GetScrollY();
    if (abs(scroll_delta) > 0.001f) {
        orbit_radius_ -= scroll_delta * scroll_sensitivity_;
    }

    // WASD 键移动目标点
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

    updateOrbitCamera();
}

std::string MaterialDemo::GetName() const {
    return "Material Demo";
}

void MaterialDemo::createBasicTextures() {
    glGenTextures(1, &texture1);
    glBindTexture(GL_TEXTURE_2D, texture1);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load("container2_specular.png", &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cout << "Failed to load texture1" << std::endl;
    }
    stbi_image_free(data);

    shader->use();
    shader->setInt("texture1", 0);
}

void MaterialDemo::createMaterialTextures() {
    // 加载漫反射贴图
    glGenTextures(1, &diffuseMap_);
    glBindTexture(GL_TEXTURE_2D, diffuseMap_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load("container2.png", &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cout << "Failed to load diffuse texture" << std::endl;
    }
    stbi_image_free(data);

    // 加载镜面反射贴图
    glGenTextures(1, &specularMap_);
    glBindTexture(GL_TEXTURE_2D, specularMap_);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    data = stbi_load("container2_specular.png", &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cout << "Failed to load specular texture" << std::endl;
    }
    stbi_image_free(data);
}

void MaterialDemo::applyTextureSettings(unsigned int textureID, int wrapMode, bool useNearest) {
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

unsigned int MaterialDemo::createSolidColorTexture(float r, float g, float b, float a) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

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

void MaterialDemo::updateOrbitCamera() {
    if (orbit_pitch_ > 89.0f) orbit_pitch_ = 89.0f;
    if (orbit_pitch_ < -89.0f) orbit_pitch_ = -89.0f;
    if (orbit_radius_ < 1.0f) orbit_radius_ = 1.0f;
    if (orbit_radius_ > 10.0f) orbit_radius_ = 10.0f;

    float x = orbit_radius_ * cos(glm::radians(orbit_pitch_)) * cos(glm::radians(orbit_yaw_));
    float y = orbit_radius_ * sin(glm::radians(orbit_pitch_));
    float z = orbit_radius_ * cos(glm::radians(orbit_pitch_)) * sin(glm::radians(orbit_yaw_));

    cameraPos = orbit_target_ + glm::vec3(x, y, z);
    cameraFront = glm::normalize(orbit_target_ - cameraPos);
}