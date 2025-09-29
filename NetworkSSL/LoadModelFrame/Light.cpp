#include "Light.h"
#include "shader.h"
#include <sstream>

std::shared_ptr<Light> LightManager::CreateDirectionalLight() {
    auto light = std::make_shared<Light>(LightType::DIRECTIONAL);
    lights.push_back(light);
    return light;
}

std::shared_ptr<Light> LightManager::CreatePointLight(const glm::vec3& pos) {
    auto light = std::make_shared<Light>(LightType::POINT);
    light->position = pos;
    lights.push_back(light);
    return light;
}

std::shared_ptr<Light> LightManager::CreateSpotLight(const glm::vec3& pos, const glm::vec3& dir) {
    auto light = std::make_shared<Light>(LightType::SPOT);
    light->position = pos;
    light->direction = dir;
    lights.push_back(light);
    return light;
}

void LightManager::RemoveLight(std::shared_ptr<Light> light) {
    lights.erase(std::remove(lights.begin(), lights.end(), light), lights.end());
}

void LightManager::Clear() {
    lights.clear();
}

void LightManager::ApplyLightsToShader(Shader& shader) const {
    shader.use();

    int dirLightCount = 0, pointLightCount = 0, spotLightCount = 0;

    for (const auto& light : lights) {
        if (!light->enabled) continue;

        std::string baseName;
        int index = 0;

        switch (light->type) {
        case LightType::DIRECTIONAL:
            baseName = "dirLights[" + std::to_string(dirLightCount++) + "]";
            shader.setVec3(baseName + ".direction", light->direction);
            break;

        case LightType::POINT:
            baseName = "pointLights[" + std::to_string(pointLightCount++) + "]";
            shader.setVec3(baseName + ".position", light->position);
            shader.setFloat(baseName + ".constant", light->constant);
            shader.setFloat(baseName + ".linear", light->linear);
            shader.setFloat(baseName + ".quadratic", light->quadratic);
            break;

        case LightType::SPOT:
            baseName = "spotLights[" + std::to_string(spotLightCount++) + "]";
            shader.setVec3(baseName + ".position", light->position);
            shader.setVec3(baseName + ".direction", light->direction);
            shader.setFloat(baseName + ".cutOff", light->cutOff);
            shader.setFloat(baseName + ".outerCutOff", light->outerCutOff);
            break;
        }

        shader.setVec3(baseName + ".ambient", light->ambient);
        shader.setVec3(baseName + ".diffuse", light->diffuse);
        shader.setVec3(baseName + ".specular", light->specular);
    }

    shader.setInt("dirLightCount", dirLightCount);
    shader.setInt("pointLightCount", pointLightCount);
    shader.setInt("spotLightCount", spotLightCount);
}