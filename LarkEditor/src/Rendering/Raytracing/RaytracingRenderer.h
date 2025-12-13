#pragma once
#include "RaytracingRendererBase.h"
#include <memory>

class RaytracingRendererBase;

class RaytracingRenderer
{
public:
    static bool Initialize();
    static void Shutdown();
    static bool IsInitialized();

    static void UploadScene(const RayTracingScene& scene);

    static void AddGeometryToScene(
        RayTracingScene& scene,
        const content_tools::scene* geometry,
        const glm::mat4& transform = glm::mat4(1.0f),
        uint32_t materialId = 0);

    static uint32_t AddMaterial(RayTracingScene& scene, const PBRMaterial& material);
    static void AddLight(RayTracingScene& scene, const RaytracingLight& light);

    static void Render(
        const glm::vec3& cameraPos,
        const glm::vec3& cameraFront,
        const glm::vec3& cameraUp,
        float fov,
        float aspectRatio,
        int viewportWidth,
        int viewportHeight);

    static int GetTriangleCount();
    static int GetMaterialCount();

private:
    static std::unique_ptr<RaytracingRendererBase> s_Implementation;
    static RaytracingRendererBase* GetImpl();
};