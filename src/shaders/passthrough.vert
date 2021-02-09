#version 450

layout( push_constant ) uniform PushConstantObject
{
    mat4 vpMatrix;
} pco;

layout(binding = 0) uniform UniformBufferObject
{
    mat4 model;
} ubo;

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNorm;

layout(location = 0) out vec3 outColor;

void main() {
    gl_Position = pco.vpMatrix * ubo.model * vec4(inPos, 1.0);
    outColor = inNorm / 2.0 + vec3(0.5);
}