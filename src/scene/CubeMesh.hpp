#pragma once

#include "renderer/Vertex.hpp"

#include <cstdint>
#include <vector>

class CubeMesh {
public:
    static const std::vector<Vertex>& vertices();
    static const std::vector<uint16_t>& indices();
};
