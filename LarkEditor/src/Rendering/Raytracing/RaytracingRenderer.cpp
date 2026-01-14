#include "RaytracingRenderer.h"

#ifdef __APPLE__
    #include "RaytracingRendererTBO.h"
    using PlatformRenderer = RaytracingRendererTBO;
#else
#include "RaytracingRendererSSBO.h"
using PlatformRenderer = RaytracingRendererSSBO;
#endif

std::unique_ptr<RaytracingRendererBase> RaytracingRenderer::s_Implementation = nullptr;

RaytracingRendererBase* RaytracingRenderer::GetImpl()
{
    if (!s_Implementation)
    {
        s_Implementation = std::make_unique<PlatformRenderer>();
    }
    return s_Implementation.get();
}

bool RaytracingRenderer::Initialize()
{
    printf("[RaytracingRenderer] Initializing with backend: %s\n",
#ifdef __APPLE__
           "TBO (macOS)"
#else
           "SSBO (Windows/Linux)"
#endif
    );
    return GetImpl()->Initialize();
}

void RaytracingRenderer::Shutdown()
{
    if (s_Implementation)
    {
        s_Implementation->Shutdown();
        s_Implementation.reset();
    }
}

bool RaytracingRenderer::IsInitialized()
{
    return s_Implementation && s_Implementation->IsInitialized();
}

void RaytracingRenderer::UploadScene(const RayTracingScene& scene)
{
    GetImpl()->UploadScene(scene);
}

void RaytracingRenderer::Render(
    const glm::vec3& cameraPos,
    const glm::vec3& cameraFront,
    const glm::vec3& cameraUp,
    float fov,
    float aspectRatio,
    int viewportWidth,
    int viewportHeight)
{
    GetImpl()->Render(cameraPos, cameraFront, cameraUp, fov, aspectRatio,
                      viewportWidth, viewportHeight);
}

int RaytracingRenderer::GetTriangleCount()
{
    return GetImpl()->GetTriangleCount();
}

int RaytracingRenderer::GetMaterialCount()
{
    return GetImpl()->GetMaterialCount();
}

// These helper functions remain the same (they're just data manipulation)
// TODO: Maybe move those to Utils
void RaytracingRenderer::AddGeometryToScene(
    RayTracingScene& scene,
    const content_tools::scene* geometry,
    const glm::mat4& transform,
    uint32_t materialId)
{
    if (!geometry) return;

    auto triangles = GeometryConverter::ConvertFromGeometry(geometry, materialId);

    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));

    for (auto& tri : triangles)
    {
        glm::vec4 v0 = transform * glm::vec4(glm::vec3(tri.v0), 1.0f);
        glm::vec4 v1 = transform * glm::vec4(glm::vec3(tri.v1), 1.0f);
        glm::vec4 v2 = transform * glm::vec4(glm::vec3(tri.v2), 1.0f);

        tri.v0 = glm::vec4(glm::vec3(v0), 0.0f);
        tri.v1 = glm::vec4(glm::vec3(v1), 0.0f);
        tri.v2 = glm::vec4(glm::vec3(v2), 0.0f);

        tri.n0 = glm::vec4(glm::normalize(normalMatrix * glm::vec3(tri.n0)), 0.0f);
        tri.n1 = glm::vec4(glm::normalize(normalMatrix * glm::vec3(tri.n1)), 0.0f);
        tri.n2 = glm::vec4(glm::normalize(normalMatrix * glm::vec3(tri.n2)), 0.0f);
    }

    scene.triangles.insert(scene.triangles.end(), triangles.begin(), triangles.end());
}

uint32_t RaytracingRenderer::AddMaterial(RayTracingScene& scene, const PBRMaterial& material)
{
    uint32_t index = static_cast<uint32_t>(scene.materials.size());
    scene.materials.push_back(material);
    return index;
}
