#pragma once
#include <vector>
#include "GeometryConverter.h"
#include "RaytracingLight.h"
#include "Rendering/Materials/PBRMaterial.h"

struct RayTracingScene
{
    std::vector<Triangle> triangles;
    std::vector<PBRMaterial> materials;
    std::vector<RaytracingLight> lights;
};