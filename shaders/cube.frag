#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragWorldPos;
layout(location = 2) in vec3 fragNormal;
layout(location = 3) in float fragEmissive;
layout(location = 0) out vec4 outColor;

layout(binding = 0) uniform UniformBufferObject {
    mat4 view;
    mat4 proj;
    vec4 sunWorldPosition;
} ubo;

void main() {
    vec3 normal = normalize(fragNormal);
    vec3 lightVector = ubo.sunWorldPosition.xyz - fragWorldPos;
    float distanceSquared = max(dot(lightVector, lightVector), 1.0);
    vec3 lightDirection = normalize(lightVector);

    const float oneAstronomicalUnitMeters = 149597870700.0;
    float irradianceScale = (oneAstronomicalUnitMeters * oneAstronomicalUnitMeters) / distanceSquared;

    float ambient = 0.01;
    float diffuse = max(dot(normal, lightDirection), 0.0) * irradianceScale;

    vec3 reflected = fragColor * (ambient + diffuse);
    vec3 emissive = fragColor * fragEmissive;
    vec3 hdrColor = reflected + emissive;
    vec3 finalColor = hdrColor / (vec3(1.0) + hdrColor);
    outColor = vec4(finalColor, 1.0);
}
