#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragWorldPos;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out float fragEmissive;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 sunWorldPosition;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 tint;
    vec4 material;
} pushConstants;

void main() {
    vec4 worldPos = pushConstants.model * vec4(inPosition, 1.0);
    mat3 normalMatrix = transpose(inverse(mat3(pushConstants.model)));

    gl_Position = ubo.proj * ubo.view * worldPos;
    fragColor = pushConstants.tint.rgb;
    fragWorldPos = worldPos.xyz;
    fragNormal = normalize(normalMatrix * inPosition);
    fragEmissive = pushConstants.material.x;
}
