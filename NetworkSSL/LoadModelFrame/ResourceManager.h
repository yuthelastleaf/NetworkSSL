#pragma once
#include <map>
#include <memory>
#include <string>

class Shader;
class Model;

class ResourceManager {
private:
    static std::map<std::string, std::shared_ptr<Shader>> shaders;
    static std::map<std::string, std::shared_ptr<Model>> models;
    static std::map<std::string, unsigned int> textures;

public:
    // ��ɫ������
    static std::shared_ptr<Shader> LoadShader(const std::string& name,
        const std::string& vertexPath,
        const std::string& fragmentPath,
        const std::string& geometryPath = "");
    static std::shared_ptr<Shader> GetShader(const std::string& name);

    // ģ�͹���
    static std::shared_ptr<Model> LoadModel(const std::string& name, const std::string& path);
    static std::shared_ptr<Model> GetModel(const std::string& name);

    // �������
    static unsigned int LoadTexture(const std::string& name, const std::string& path, bool gammaCorrection = false);
    static unsigned int GetTexture(const std::string& name);

    // ������Դ
    static void Clear();
};