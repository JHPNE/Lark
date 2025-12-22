#pragma once

#include "Structures/Structures.h"
#include "glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

// Triangle structure for GPU raytracing
// In std430, vec3 is aligned to 16 bytes, so we use vec4 for proper alignment
struct Triangle
{
    glm::vec4 v0, v1, v2;        // xyz = position, w = padding (16 bytes)
    glm::vec4 n0, n1, n2;        // xyz = normal, w = padding (16 bytes)
    glm::vec2 uv0, uv1, uv2;       // (8 bytes)
    uint32_t materialId; // (4 bytes)
    float _padding1;     // (4 bytes) - align to 16 bytes
};

struct alignas(16) TriangleTBOGPU
{
};

class GeometryConverter
{
    public:
        static std::vector<Triangle> ConvertFromGeometry(const content_tools::scene* geometry, uint32_t materialId = 0);
};