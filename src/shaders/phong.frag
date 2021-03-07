#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 inNormal;
layout(location = 0) out vec4 outColor;

void main() {
    float lambertFactor = 0.7;
    float ambientFactor = 0.3;

    float lambert = max(dot(normalize(inNormal), vec3(1.0, 0.0, 0.0)), 0.0);
    float ambient = 1.0;
    outColor = vec4(vec3(lambert * lambertFactor + ambient * ambientFactor), 1.0);
}