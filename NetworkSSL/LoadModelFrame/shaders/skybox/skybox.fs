#version 330 core
out vec4 FragColor;

in vec3 TexCoords;
uniform samplerCube skybox;  // 确保名称一致

void main() {    
    FragColor = texture(skybox, TexCoords);
}