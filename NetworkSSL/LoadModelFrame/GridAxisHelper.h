#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class Shader;

struct GridConfig {
    bool enabled = true;
    float size = 20.0f;          // 网格总大小（每边）
    float spacing = 1.0f;        // 网格间距
    glm::vec3 color = glm::vec3(0.3f, 0.3f, 0.3f);  // 默认深灰色
    glm::vec3 centerLineColor = glm::vec3(0.5f, 0.5f, 0.5f);  // 中心线颜色
    float lineWidth = 1.0f;
    float centerLineWidth = 2.0f;
};

struct AxisConfig {
    bool enabled = true;
    float length = 5.0f;         // 轴长度
    float arrowSize = 0.05f;      // 箭头大小
    glm::vec3 xColor = glm::vec3(1.0f, 0.0f, 0.0f);  // X轴红色
    glm::vec3 yColor = glm::vec3(0.0f, 1.0f, 0.0f);  // Y轴绿色
    glm::vec3 zColor = glm::vec3(0.0f, 0.0f, 1.0f);  // Z轴蓝色
    float lineWidth = 2.0f;
};

class GridAxisHelper {
private:
    // 网格相关
    unsigned int gridVAO, gridVBO;
    std::vector<float> gridVertices;
    int gridVertexCount;
    std::shared_ptr<Shader> gridShader;

    // 坐标轴相关
    unsigned int axisVAO, axisVBO;
    unsigned int arrowVAO, arrowVBO, arrowEBO;
    std::vector<float> axisVertices;
    int axisVertexCount;
    std::shared_ptr<Shader> axisShader;

    GridConfig gridConfig;
    AxisConfig axisConfig;

    void setupGridGeometry();
    void setupAxisGeometry();
    void createArrowGeometry();
    void createShaders();

public:
    GridAxisHelper();
    ~GridAxisHelper();

    void Initialize();
    void Render(const glm::mat4& view, const glm::mat4& projection);
    void Cleanup();

    // 配置接口
    GridConfig& GetGridConfig() { return gridConfig; }
    AxisConfig& GetAxisConfig() { return axisConfig; }

    void SetGridEnabled(bool enabled) { gridConfig.enabled = enabled; }
    void SetAxisEnabled(bool enabled) { axisConfig.enabled = enabled; }

    void SetGridSize(float size) {
        gridConfig.size = size;
        setupGridGeometry();
    }

    void SetGridSpacing(float spacing) {
        gridConfig.spacing = spacing;
        setupGridGeometry();
    }

    void SetAxisLength(float length) {
        axisConfig.length = length;
        setupAxisGeometry();
    }

    // 根据相机距离自动调整网格
    void UpdateGridForCamera(float cameraDistance);
};