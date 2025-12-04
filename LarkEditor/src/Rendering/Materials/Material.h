#pragma once
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include "glad/glad.h"

struct Material
{
    glm::vec3 albedo;
    float roughness;
    float metallic;

    glm::vec3 normalScale;
    float ao;

    glm::vec3 emissive;
    float ior;
    float transparency;
};

struct Glas : Material
{
};

