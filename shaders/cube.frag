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
    vec4 shadowBodySpheres[16];
    vec4 shadowMeta;
} ubo;

layout(push_constant) uniform PushConstants {
    mat4 model;
    vec4 tint;
    vec4 material;
    vec4 objectInfo;
} pushConstants;

bool segmentIntersectsSphere(vec3 segmentStart, vec3 segmentEnd, vec3 sphereCenter, float sphereRadius) {
    vec3 direction = segmentEnd - segmentStart;
    vec3 m = segmentStart - sphereCenter;
    float a = dot(direction, direction);
    float b = 2.0 * dot(m, direction);
    float c = dot(m, m) - sphereRadius * sphereRadius;

    float discriminant = b * b - 4.0 * a * c;
    if (discriminant < 0.0 || a <= 1e-9) {
        return false;
    }

    float sqrtDiscriminant = sqrt(discriminant);
    float invDenominator = 0.5 / a;
    float t1 = (-b - sqrtDiscriminant) * invDenominator;
    float t2 = (-b + sqrtDiscriminant) * invDenominator;

    return (t1 > 0.0001 && t1 < 0.9999) || (t2 > 0.0001 && t2 < 0.9999);
}

void main() {
    vec3 normal = normalize(fragNormal);
    vec3 lightVector = ubo.sunWorldPosition.xyz - fragWorldPos;
    float distanceSquared = max(dot(lightVector, lightVector), 1.0);
    vec3 lightDirection = normalize(lightVector);

    const float oneAstronomicalUnitMeters = 149597870700.0;
    float irradianceScale = (oneAstronomicalUnitMeters * oneAstronomicalUnitMeters) / distanceSquared;

    float shadowFactor = 1.0;
    int bodyCount = int(ubo.shadowMeta.x + 0.5);
    int sunIndex = int(ubo.shadowMeta.y + 0.5);
    int selfIndex = int(pushConstants.objectInfo.x + 0.5);
    for (int bodyIndex = 0; bodyIndex < bodyCount; ++bodyIndex) {
        if (bodyIndex == selfIndex || bodyIndex == sunIndex) {
            continue;
        }

        vec4 sphere = ubo.shadowBodySpheres[bodyIndex];
        if (segmentIntersectsSphere(ubo.sunWorldPosition.xyz, fragWorldPos, sphere.xyz, sphere.w)) {
            shadowFactor = 0.0;
            break;
        }
    }

    float ambient = 0.01;
    float diffuse = max(dot(normal, lightDirection), 0.0) * irradianceScale * shadowFactor;

    vec3 reflected = fragColor * (ambient + diffuse);
    vec3 emissive = fragColor * fragEmissive;
    vec3 hdrColor = reflected + emissive;
    vec3 finalColor = hdrColor / (vec3(1.0) + hdrColor);
    outColor = vec4(finalColor, 1.0);
}
