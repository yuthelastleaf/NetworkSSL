#include "GridAxisHelper.h"
#include "shader.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

GridAxisHelper::GridAxisHelper()
    : gridVAO(0), gridVBO(0), axisVAO(0), axisVBO(0),
    arrowVAO(0), arrowVBO(0), arrowEBO(0),
    gridVertexCount(0), axisVertexCount(0) {
}

GridAxisHelper::~GridAxisHelper() {
    Cleanup();
}

void GridAxisHelper::Initialize() {
    std::cout << "Initializing GridAxisHelper..." << std::endl;
    createShaders();
    setupGridGeometry();
    setupAxisGeometry();
    createArrowGeometry();
    std::cout << "GridAxisHelper initialized successfully!" << std::endl;
}

void GridAxisHelper::createShaders() {
    // 简单的颜色着色器
    const char* vertexShaderSource = R"(
        #version 330 core
        layout (location = 0) in vec3 aPos;
        layout (location = 1) in vec3 aColor;
        
        uniform mat4 view;
        uniform mat4 projection;
        uniform mat4 model;
        
        out vec3 vertexColor;
        
        void main() {
            vertexColor = aColor;
            gl_Position = projection * view * model * vec4(aPos, 1.0);
        }
    )";

    const char* fragmentShaderSource = R"(
        #version 330 core
        in vec3 vertexColor;
        out vec4 FragColor;
        
        void main() {
            FragColor = vec4(vertexColor, 1.0);
        }
    )";

    gridShader = std::make_shared<Shader>(vertexShaderSource, fragmentShaderSource, true);
    axisShader = std::make_shared<Shader>(vertexShaderSource, fragmentShaderSource, true);
}

void GridAxisHelper::setupGridGeometry() {
    gridVertices.clear();

    float halfSize = gridConfig.size / 2.0f;
    int numLines = static_cast<int>(gridConfig.size / gridConfig.spacing);

    // 生成平行于X轴的线（沿Z方向）
    for (int i = -numLines; i <= numLines; ++i) {
        float z = i * gridConfig.spacing;

        // 判断是否是中心线
        bool isCenterLine = (i == 0);
        glm::vec3 color = isCenterLine ? gridConfig.centerLineColor : gridConfig.color;

        // 线的起点
        gridVertices.push_back(-halfSize); // x
        gridVertices.push_back(0.0f);       // y
        gridVertices.push_back(z);          // z
        gridVertices.push_back(color.r);
        gridVertices.push_back(color.g);
        gridVertices.push_back(color.b);

        // 线的终点
        gridVertices.push_back(halfSize);   // x
        gridVertices.push_back(0.0f);       // y
        gridVertices.push_back(z);          // z
        gridVertices.push_back(color.r);
        gridVertices.push_back(color.g);
        gridVertices.push_back(color.b);
    }

    // 生成平行于Z轴的线（沿X方向）
    for (int i = -numLines; i <= numLines; ++i) {
        float x = i * gridConfig.spacing;

        bool isCenterLine = (i == 0);
        glm::vec3 color = isCenterLine ? gridConfig.centerLineColor : gridConfig.color;

        // 线的起点
        gridVertices.push_back(x);          // x
        gridVertices.push_back(0.0f);       // y
        gridVertices.push_back(-halfSize);  // z
        gridVertices.push_back(color.r);
        gridVertices.push_back(color.g);
        gridVertices.push_back(color.b);

        // 线的终点
        gridVertices.push_back(x);          // x
        gridVertices.push_back(0.0f);       // y
        gridVertices.push_back(halfSize);   // z
        gridVertices.push_back(color.r);
        gridVertices.push_back(color.g);
        gridVertices.push_back(color.b);
    }

    gridVertexCount = gridVertices.size() / 6;

    // 创建VAO/VBO
    if (gridVAO == 0) {
        glGenVertexArrays(1, &gridVAO);
        glGenBuffers(1, &gridVBO);
    }

    glBindVertexArray(gridVAO);
    glBindBuffer(GL_ARRAY_BUFFER, gridVBO);
    glBufferData(GL_ARRAY_BUFFER, gridVertices.size() * sizeof(float),
        gridVertices.data(), GL_STATIC_DRAW);

    // 位置属性
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // 颜色属性
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
        (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void GridAxisHelper::setupAxisGeometry() {
    axisVertices.clear();

    float len = axisConfig.length;

    // X轴（红色）- 从原点到正方向
    axisVertices.insert(axisVertices.end(), {
        0.0f, 0.0f, 0.0f, axisConfig.xColor.r, axisConfig.xColor.g, axisConfig.xColor.b,
        len, 0.0f, 0.0f, axisConfig.xColor.r, axisConfig.xColor.g, axisConfig.xColor.b
        });

    // Y轴（绿色）
    axisVertices.insert(axisVertices.end(), {
        0.0f, 0.0f, 0.0f, axisConfig.yColor.r, axisConfig.yColor.g, axisConfig.yColor.b,
        0.0f, len, 0.0f, axisConfig.yColor.r, axisConfig.yColor.g, axisConfig.yColor.b
        });

    // Z轴（蓝色）
    axisVertices.insert(axisVertices.end(), {
        0.0f, 0.0f, 0.0f, axisConfig.zColor.r, axisConfig.zColor.g, axisConfig.zColor.b,
        0.0f, 0.0f, len, axisConfig.zColor.r, axisConfig.zColor.g, axisConfig.zColor.b
        });

    axisVertexCount = axisVertices.size() / 6;

    if (axisVAO == 0) {
        glGenVertexArrays(1, &axisVAO);
        glGenBuffers(1, &axisVBO);
    }

    glBindVertexArray(axisVAO);
    glBindBuffer(GL_ARRAY_BUFFER, axisVBO);
    glBufferData(GL_ARRAY_BUFFER, axisVertices.size() * sizeof(float),
        axisVertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
        (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void GridAxisHelper::createArrowGeometry() {
    // 箭头锥体（使用三角形扇形）
    float arrowSize = axisConfig.arrowSize;
    float arrowLength = arrowSize * 1.5f;
    int segments = 8; // 锥体的分段数

    std::vector<float> arrowVerts;
    std::vector<unsigned int> arrowIndices;

    // 锥体顶点（在局部坐标系中，指向+X方向）
    arrowVerts.insert(arrowVerts.end(), {
        arrowLength, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f  // 尖端
        });

    // 底部圆形顶点
    for (int i = 0; i <= segments; ++i) {
        float angle = 2.0f * glm::pi<float>() * i / segments;
        float y = arrowSize * cos(angle);
        float z = arrowSize * sin(angle);
        arrowVerts.insert(arrowVerts.end(), {
            0.0f, y, z, 1.0f, 1.0f, 1.0f
            });
    }

    // 创建三角形扇形索引
    for (int i = 1; i <= segments; ++i) {
        arrowIndices.push_back(0);  // 尖端
        arrowIndices.push_back(i);
        arrowIndices.push_back(i + 1);
    }

    glGenVertexArrays(1, &arrowVAO);
    glGenBuffers(1, &arrowVBO);
    glGenBuffers(1, &arrowEBO);

    glBindVertexArray(arrowVAO);

    glBindBuffer(GL_ARRAY_BUFFER, arrowVBO);
    glBufferData(GL_ARRAY_BUFFER, arrowVerts.size() * sizeof(float),
        arrowVerts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, arrowEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, arrowIndices.size() * sizeof(unsigned int),
        arrowIndices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
        (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void GridAxisHelper::UpdateGridForCamera(float cameraDistance) {
    // 根据相机距离自动调整网格间距，保持合适的密度
    if (cameraDistance < 5.0f) {
        gridConfig.spacing = 0.5f;
        gridConfig.size = 10.0f;
    }
    else if (cameraDistance < 20.0f) {
        gridConfig.spacing = 1.0f;
        gridConfig.size = 20.0f;
    }
    else if (cameraDistance < 50.0f) {
        gridConfig.spacing = 2.0f;
        gridConfig.size = 50.0f;
    }
    else {
        gridConfig.spacing = 5.0f;
        gridConfig.size = 100.0f;
    }
    setupGridGeometry();
}

void GridAxisHelper::Render(const glm::mat4& view, const glm::mat4& projection) {
    glm::mat4 model = glm::mat4(1.0f);

    // 渲染网格
    if (gridConfig.enabled) {
        gridShader->use();
        gridShader->setMat4("view", view);
        gridShader->setMat4("projection", projection);
        gridShader->setMat4("model", model);

        glLineWidth(gridConfig.lineWidth);
        glBindVertexArray(gridVAO);
        glDrawArrays(GL_LINES, 0, gridVertexCount);
    }

    // 渲染坐标轴
    if (axisConfig.enabled) {
        axisShader->use();
        axisShader->setMat4("view", view);
        axisShader->setMat4("projection", projection);
        axisShader->setMat4("model", model);

        // 绘制轴线
        glLineWidth(axisConfig.lineWidth);
        glBindVertexArray(axisVAO);
        glDrawArrays(GL_LINES, 0, axisVertexCount);

        // 绘制箭头
        glBindVertexArray(arrowVAO);

        // X轴箭头（红色）
        glm::mat4 arrowModel = glm::translate(model, glm::vec3(axisConfig.length, 0.0f, 0.0f));
        axisShader->setMat4("model", arrowModel);
        axisShader->use(); // 确保uniform更新
        glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_INT, 0);

        // Y轴箭头（绿色）- 旋转90度朝向+Y
        arrowModel = glm::translate(model, glm::vec3(0.0f, axisConfig.length, 0.0f));
        arrowModel = glm::rotate(arrowModel, glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        axisShader->setMat4("model", arrowModel);
        glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_INT, 0);

        // Z轴箭头（蓝色）- 旋转-90度朝向+Z
        arrowModel = glm::translate(model, glm::vec3(0.0f, 0.0f, axisConfig.length));
        arrowModel = glm::rotate(arrowModel, glm::radians(-90.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        axisShader->setMat4("model", arrowModel);
        glDrawElements(GL_TRIANGLES, 24, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
    glLineWidth(1.0f); // 恢复默认线宽
}

void GridAxisHelper::Cleanup() {
    if (gridVAO) glDeleteVertexArrays(1, &gridVAO);
    if (gridVBO) glDeleteBuffers(1, &gridVBO);
    if (axisVAO) glDeleteVertexArrays(1, &axisVAO);
    if (axisVBO) glDeleteBuffers(1, &axisVBO);
    if (arrowVAO) glDeleteVertexArrays(1, &arrowVAO);
    if (arrowVBO) glDeleteBuffers(1, &arrowVBO);
    if (arrowEBO) glDeleteBuffers(1, &arrowEBO);

    gridVAO = gridVBO = axisVAO = axisVBO = arrowVAO = arrowVBO = arrowEBO = 0;
}