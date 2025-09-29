#pragma once
#include <glm/glm.hpp>
#include <memory>
#include <vector>

class Shader;

struct GridConfig {
    bool enabled = true;
    float size = 20.0f;          // �����ܴ�С��ÿ�ߣ�
    float spacing = 1.0f;        // ������
    glm::vec3 color = glm::vec3(0.3f, 0.3f, 0.3f);  // Ĭ�����ɫ
    glm::vec3 centerLineColor = glm::vec3(0.5f, 0.5f, 0.5f);  // ��������ɫ
    float lineWidth = 1.0f;
    float centerLineWidth = 2.0f;
};

struct AxisConfig {
    bool enabled = true;
    float length = 5.0f;         // �᳤��
    float arrowSize = 0.05f;      // ��ͷ��С
    glm::vec3 xColor = glm::vec3(1.0f, 0.0f, 0.0f);  // X���ɫ
    glm::vec3 yColor = glm::vec3(0.0f, 1.0f, 0.0f);  // Y����ɫ
    glm::vec3 zColor = glm::vec3(0.0f, 0.0f, 1.0f);  // Z����ɫ
    float lineWidth = 2.0f;
};

class GridAxisHelper {
private:
    // �������
    unsigned int gridVAO, gridVBO;
    std::vector<float> gridVertices;
    int gridVertexCount;
    std::shared_ptr<Shader> gridShader;

    // ���������
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

    // ���ýӿ�
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

    // ������������Զ���������
    void UpdateGridForCamera(float cameraDistance);
};