#pragma once
#include <vector>
#include "GeometryConverter.h"
#include "Rendering/Materials/PBRMaterial.h"

struct RayTracingScene
{
    std::vector<GPUTriangle> triangles;
    std::vector<PBRMaterial> materials;

    // Light missing for now
};