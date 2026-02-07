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
    glm::vec4 v0, v1, v2;
    glm::vec4 n0, n1, n2;  // xyz = normal, w unused
    glm::vec4 uvData0;  // xy = uv0, zw = uv1
    glm::vec4 uvData1;  // xy = uv2, z = materialId (as float), w unused

    static std::vector<TriangleTBOGPU> FromTBO(const std::vector<Triangle>& triangles)
    {
        std::vector<TriangleTBOGPU> tboData;
        tboData.reserve(triangles.size());

        for (const auto& tri : triangles)
        {
            TriangleTBOGPU data;
            data.v0 = tri.v0;
            data.v1 = tri.v1;
            data.v2 = tri.v2;
            data.n0 = tri.n0;
            data.n1 = tri.n1;
            data.n2 = tri.n2;
            data.uvData0 = glm::vec4(tri.uv0, tri.uv1);
            data.uvData1 = glm::vec4(tri.uv2, glm::uintBitsToFloat(tri.materialId), 0.0f);
            tboData.push_back(data);
        }

        return tboData;
    }
};

class GeometryConverter
{
    public:
        static std::vector<Triangle> ConvertFromGeometry(const content_tools::scene* geometry, uint32_t materialId = 0);
};