// outline.fs
#version 330 core
out vec4 FragColor;

uniform vec3 outlineColor;
uniform float outlineAlpha;  // 透明度

void main()
{
    FragColor = vec4(outlineColor, outlineAlpha);
}
