#include "ResourceManager.h"
#include "shader.h"
#include "Model.h"
#include <iostream>
#include <glad/glad.h>
#include "stb_image.h"

std::map<std::string, std::shared_ptr<Shader>> ResourceManager::shaders;
std::map<std::string, std::shared_ptr<Model>> ResourceManager::models;
std::map<std::string, unsigned int> ResourceManager::textures;

std::shared_ptr<Shader> ResourceManager::LoadShader(const std::string& name,
    const std::string& vertexPath,
    const std::string& fragmentPath,
    const std::string& geometryPath) {
    auto shader = std::make_shared<Shader>(vertexPath.c_str(), fragmentPath.c_str());
    shaders[name] = shader;
    return shader;
}

std::shared_ptr<Shader> ResourceManager::GetShader(const std::string& name) {
    auto it = shaders.find(name);
    if (it != shaders.end()) {
        return it->second;
    }
    std::cout << "Shader not found: " << name << std::endl;
    return nullptr;
}

std::shared_ptr<Model> ResourceManager::LoadModel(const std::string& name, const std::string& path) {
    auto model = std::make_shared<Model>(path);
    models[name] = model;
    return model;
}

std::shared_ptr<Model> ResourceManager::GetModel(const std::string& name) {
    auto it = models.find(name);
    if (it != models.end()) {
        return it->second;
    }
    std::cout << "Model not found: " << name << std::endl;
    return nullptr;
}

unsigned int ResourceManager::LoadTexture(const std::string& name, const std::string& path, bool gammaCorrection) {
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &nrComponents, 0);

    if (data) {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
        textures[name] = textureID;
    }
    else {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

unsigned int ResourceManager::GetTexture(const std::string& name) {
    auto it = textures.find(name);
    if (it != textures.end()) {
        return it->second;
    }
    std::cout << "Texture not found: " << name << std::endl;
    return 0;
}

void ResourceManager::Clear() {
    shaders.clear();
    models.clear();

    for (auto& texture : textures) {
        glDeleteTextures(1, &texture.second);
    }
    textures.clear();
}