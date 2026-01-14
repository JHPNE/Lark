// RaytracingRendererBase.h
#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "GeometryConverter.h"
#include "../Materials/PBRMaterial.h"

struct RayTracingScene
{
    std::vector<Triangle> triangles;
    std::vector<PBRMaterial> materials;

    glm::vec3 backgroundColor = glm::vec3(0.1f, 0.1f, 0.2f);
    glm::vec3 ambientColor = glm::vec3(0.1f, 0.1f, 0.15f);
};

/**
 * @brief Base class for platform-specific raytracing implementations
 */
class RaytracingRendererBase
{
public:
    virtual ~RaytracingRendererBase() = default;

    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;
    virtual bool IsInitialized() const = 0;

    virtual void UploadScene(const RayTracingScene& scene) = 0;

    virtual void Render(
        const glm::vec3& cameraPos,
        const glm::vec3& cameraFront,
        const glm::vec3& cameraUp,
        float fov,
        float aspectRatio,
        int viewportWidth,
        int viewportHeight) = 0;

    virtual int GetTriangleCount() const = 0;
    virtual int GetMaterialCount() const = 0;
};