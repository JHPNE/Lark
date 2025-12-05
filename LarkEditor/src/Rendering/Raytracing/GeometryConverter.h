#pragma once

#include "Structures/Structures.h"
#include "glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

struct Triangle
{
    glm::vec3 v0, v1, v2; // 36 by
    glm::vec3 n0, n1, n2; // 36 by
    glm::vec2 uv0, uv1, uv2; // 24 by
    uint32_t materialId; // might need to add ids later 4 by
    float _padding1, _padding2, _padding3; // 12 by
};

class GeometryConverter
{
    public:
        static std::vector<Triangle> ConvertFromGeometry(const content_tools::scene* geometry, uint32_t materialId = 0);
};