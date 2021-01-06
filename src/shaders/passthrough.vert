#version 450

layout(location = 0) in vec3 inPos;

layout(location = 0) out vec3 outColor;

void main() {
    gl_Position = vec4(inPos, 1.0);
    outColor = vec3(1.0f, 1.0f, 1.0f);
}