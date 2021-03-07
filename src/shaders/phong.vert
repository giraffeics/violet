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

layout(location = 0) out vec3 outNormal;

void main() {
    gl_Position = pco.vpMatrix * ubo.model * vec4(inPos, 1.0);
    
    vec4 normal4 = ubo.model * vec4(inNorm, 0.0);
    outNormal = normalize(vec3(normal4.x, normal4.y, normal4.z));
}