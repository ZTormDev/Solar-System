#pragma once

#include "renderer/Vertex.hpp"

#include <cstdint>
#include <vector>

struct MeshData {
    std::vector<Vertex> vertices;
    std::vector<uint16_t> indices;
};

class SphereMesh {
public:
    static MeshData createUvSphere(uint32_t latitudeSegments, uint32_t longitudeSegments);
};
