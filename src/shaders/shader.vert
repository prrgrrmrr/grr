#version 450

layout(binding = 0) uniform UniformBufferObject {
    mat4x4 model;
    mat4x4 view;
    mat4x4 projection;
} ubo;

layout(location = 0) in vec3 inPosition;

layout(location = 0) out vec3 fragColor;

void main() {
    gl_Position = ubo.projection * ubo.view * ubo.model * vec4(inPosition, 1.0);
    fragColor = vec3(0.0, 1.0, 1.0);
}