#include "scene/SphereMesh.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

MeshData SphereMesh::createUvSphere(uint32_t latitudeSegments, uint32_t longitudeSegments) {
    latitudeSegments = std::max(2u, latitudeSegments);
    longitudeSegments = std::max(3u, longitudeSegments);

    const uint32_t vertexCount = (latitudeSegments + 1) * (longitudeSegments + 1);
    const uint32_t indexCount = latitudeSegments * longitudeSegments * 6;
    if (vertexCount > 65535u) {
        throw std::runtime_error("Sphere mesh exceeds 16-bit index limits for a single LOD mesh.");
    }

    MeshData mesh;
    mesh.vertices.reserve(vertexCount);
    mesh.indices.reserve(indexCount);

    constexpr float pi = 3.14159265359f;
    constexpr float tau = 6.28318530718f;

    for (uint32_t lat = 0; lat <= latitudeSegments; ++lat) {
        const float v = static_cast<float>(lat) / static_cast<float>(latitudeSegments);
        const float phi = v * pi;
        const float sinPhi = std::sin(phi);
        const float cosPhi = std::cos(phi);

        for (uint32_t lon = 0; lon <= longitudeSegments; ++lon) {
            const float u = static_cast<float>(lon) / static_cast<float>(longitudeSegments);
            const float theta = u * tau;
            const float sinTheta = std::sin(theta);
            const float cosTheta = std::cos(theta);

            const glm::vec3 position = {
                sinPhi * cosTheta,
                sinPhi * sinTheta,
                cosPhi
            };

            const glm::vec3 color = (position * 0.5f) + glm::vec3(0.5f);
            mesh.vertices.push_back(Vertex{position, color});
        }
    }

    const uint32_t ringVertexCount = longitudeSegments + 1;
    for (uint32_t lat = 0; lat < latitudeSegments; ++lat) {
        for (uint32_t lon = 0; lon < longitudeSegments; ++lon) {
            const uint32_t current = lat * ringVertexCount + lon;
            const uint32_t next = current + ringVertexCount;

            mesh.indices.push_back(static_cast<uint16_t>(current));
            mesh.indices.push_back(static_cast<uint16_t>(next));
            mesh.indices.push_back(static_cast<uint16_t>(current + 1));

            mesh.indices.push_back(static_cast<uint16_t>(current + 1));
            mesh.indices.push_back(static_cast<uint16_t>(next));
            mesh.indices.push_back(static_cast<uint16_t>(next + 1));
        }
    }

    return mesh;
}
