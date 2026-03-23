#include "CubeMesh.hpp"

const std::vector<Vertex>& CubeMesh::vertices() {
    static const std::vector<Vertex> cubeVertices = {
        {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
        {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
        {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}},
        {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 0.0f}},
        {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f, 1.0f}},
        {{0.5f, -0.5f, 0.5f}, {0.0f, 1.0f, 1.0f}},
        {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f, 1.0f}},
        {{-0.5f, 0.5f, 0.5f}, {0.2f, 0.2f, 0.2f}}
    };

    return cubeVertices;
}

const std::vector<uint16_t>& CubeMesh::indices() {
    static const std::vector<uint16_t> cubeIndices = {
        0, 2, 1, 0, 3, 2,
        4, 5, 6, 4, 6, 7,
        0, 4, 7, 7, 3, 0,
        1, 6, 5, 6, 1, 2,
        3, 6, 2, 3, 7, 6,
        0, 1, 5, 0, 5, 4
    };

    return cubeIndices;
}
