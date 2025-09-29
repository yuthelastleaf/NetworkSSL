#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <memory>

enum class LightType {
    DIRECTIONAL,
    POINT,
    SPOT
};

struct Light {
    LightType type;
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);

    glm::vec3 ambient = glm::vec3(0.1f);
    glm::vec3 diffuse = glm::vec3(1.0f);
    glm::vec3 specular = glm::vec3(1.0f);

    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;

    float cutOff = glm::cos(glm::radians(12.5f));
    float outerCutOff = glm::cos(glm::radians(15.0f));

    bool enabled = true;

    Light(LightType t) : type(t) {}
};

class LightManager {
private:
    std::vector<std::shared_ptr<Light>> lights;

public:
    std::shared_ptr<Light> CreateDirectionalLight();
    std::shared_ptr<Light> CreatePointLight(const glm::vec3& pos);
    std::shared_ptr<Light> CreateSpotLight(const glm::vec3& pos, const glm::vec3& dir);

    void RemoveLight(std::shared_ptr<Light> light);
    void Clear();

    const std::vector<std::shared_ptr<Light>>& GetLights() const { return lights; }
    void ApplyLightsToShader(class Shader& shader) const;
};