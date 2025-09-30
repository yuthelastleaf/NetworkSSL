#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
in vec3 LightingColor;
uniform sampler2D texture_diffuse;

void main() {
    FragColor = texture(texture_diffuse, TexCoord) * vec4(LightingColor, 1.0);
}
