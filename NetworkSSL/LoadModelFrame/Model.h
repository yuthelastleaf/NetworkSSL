#pragma once
#include <string>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <assimp/material.h>

struct Vertex {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

struct Texture {
    unsigned int id;
    std::string type;  // diffuse, specular, normal, height
    std::string path;
};

class Mesh {
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<Texture> textures;

    unsigned int VAO, VBO, EBO;

public:
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<Texture> textures);
    void Draw(class Shader& shader);

private:
    void setupMesh();
};

class Model {
public:
    std::vector<Texture> textures_loaded;
    std::vector<Mesh> meshes;
    std::string directory;
    bool gammaCorrection;

public:
    Model(const std::string& path, bool gamma = false);
    void Draw(class Shader& shader);

private:
    void loadModel(const std::string& path);
    void processNode(struct aiNode* node, const struct aiScene* scene);
    Mesh processMesh(struct aiMesh* mesh, const struct aiScene* scene);
    std::vector<Texture> loadMaterialTextures(struct aiMaterial* mat, aiTextureType type, const std::string& typeName);
};

// 声明纹理加载函数
unsigned int TextureFromFile(const char* path, const std::string& directory, bool gamma = false);
