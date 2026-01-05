#pragma once
#include <glm/glm.hpp>
#include <string>
#include <memory>
#include "glad/glad.h"

struct PBRMaterial
{
    glm::vec3 albedo = glm::vec3(0.8f);
    float roughness = 0.5f;

    glm::vec3 normal = glm::vec3(0.0f, 0.0f, 1.0f);
    float ao = 1.0f;

    glm::vec3 emissive = glm::vec3(0.0f);
    float ior = 1.5f;

    float transparency = 0.0f;
    float metallic = 0.0f;
};

struct Glas : PBRMaterial
{
};

struct alignas(16) PBRMaterialGPU
{
    glm::vec4 albedoRoughness;    // xyz = albedo, w = roughness
    glm::vec4 normalAO;           // xyz = normal, w = ao
    glm::vec4 emissiveIOR;        // xyz = emissive, w = ior
    glm::vec4 transparencyMetallicPad;    // x = transparency, y = metallic, zw = unused
    
    static PBRMaterialGPU FromMaterial(const PBRMaterial& mat)
    {
        PBRMaterialGPU gpu;
        gpu.albedoRoughness = glm::vec4(mat.albedo, mat.roughness);
        gpu.normalAO = glm::vec4(mat.normal, mat.ao);
        gpu.emissiveIOR = glm::vec4(mat.emissive, mat.ior);
        gpu.transparencyMetallicPad = glm::vec4(mat.transparency, mat.metallic, 0.0f, 0.0f);
        return gpu;
    }
};



