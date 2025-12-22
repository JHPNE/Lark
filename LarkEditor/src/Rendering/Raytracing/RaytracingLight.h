#pragma once

#include "glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

enum class LightType
{
    Point,
    Directional,
    Spot,
    Area
};

struct RaytracingLight
{
    LightType type;
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
    float intensity;
    float radius;  // für Soft Shadows
};

struct alignas(16) RaytracingLightGPU
{
    glm::vec4 position;   // xyz = position, w = unused
    glm::vec4 direction;  // xyz = direction, w = unused
    glm::vec4 color;      // xyz = color, w = unused
    glm::vec4 params;     // x = intensity, y = radius, z = for type maybe later, w = unused
    
    static RaytracingLightGPU FromLight(const RaytracingLight& light)
    {
        RaytracingLightGPU gpu;
        gpu.position = glm::vec4(light.position, 0.0f);
        gpu.direction = glm::vec4(light.direction, 0.0f);
        gpu.color = glm::vec4(light.color, 0.0f);
        gpu.params = glm::vec4(light.intensity, light.radius, static_cast<float>(light.type), 0.0f);
        return gpu;
    }
};