#version 450

vec2 positions[3] = vec2[](
    vec2(0.0, -0.5),
    vec2(0.5, 0.5),
    vec2(-0.5, 0.5)
);

vec3 colors[3] = vec3[](
    vec3(0.1, 0.5, 0.9),
    vec3(0.8, 0.7, 0.1),
    vec3(1.0, 1.0, 1.0)
);

layout(location = 0) out vec3 outColor;

void main() {
    gl_Position = vec4(positions[gl_VertexIndex], 0.0, 1.0);
    outColor = colors[gl_VertexIndex];
}