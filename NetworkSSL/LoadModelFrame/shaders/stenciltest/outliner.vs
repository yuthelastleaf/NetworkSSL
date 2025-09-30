// shaders/stenciltest/outline.vs
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;  // 需要法线

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float outlineWidth;  // 轮廓宽度（世界空间单位）

void main()
{
    // 计算世界空间的法线
    mat3 normalMatrix = mat3(transpose(inverse(model)));
    vec3 worldNormal = normalize(normalMatrix * aNormal);
    
    // 沿法线方向扩展顶点位置
    vec3 worldPos = vec3(model * vec4(aPos, 1.0));
    vec3 expandedPos = worldPos + worldNormal * outlineWidth;
    
    gl_Position = projection * view * vec4(expandedPos, 1.0);
}