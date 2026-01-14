// RaytracingRendererTBO.h (for macOS)
#pragma once
#include "RaytracingRendererBase.h"
#include "glad/glad.h"
#include <random>


class RaytracingRendererTBO : public RaytracingRendererBase
{
public:
    bool Initialize() override;
    void Shutdown() override;
    bool IsInitialized() const override { return m_Initialized; }

    void UploadScene(const RayTracingScene& scene) override;

    void Render(
        const glm::vec3& cameraPos,
        const glm::vec3& cameraFront,
        const glm::vec3& cameraUp,
        float fov,
        float aspectRatio,
        int viewportWidth,
        int viewportHeight) override;

    int GetTriangleCount() const override { return m_TriangleCount; }
    int GetMaterialCount() const override { return m_MaterialCount; }

    void SetSamplesPerPixel(int samples) { m_SamplesPerPixel = samples < 1 ? 1 : samples; }
    int GetSamplesPerPixel() const { return m_SamplesPerPixel; }

private:
    bool CreateShaders();
    bool CreateFullscreenQuad();
    void CreateTBOs();

    GLuint m_ShaderProgram = 0;
    GLuint m_QuadVAO = 0;
    GLuint m_QuadVBO = 0;

    // TBO resources
    GLuint m_TriangleTBO = 0;
    GLuint m_TriangleBuffer = 0;
    GLuint m_MaterialTBO = 0;
    GLuint m_MaterialBuffer = 0;

    int m_TriangleCount = 0;
    int m_MaterialCount = 0;
    bool m_Initialized = false;

    int m_SamplesPerPixel = 4;
    std::mt19937 m_Rng{std::random_device{}()};
    std::uniform_real_distribution<float> m_SeedDist{0.0f, 1000000.0f};

    static const std::string s_VertexShaderPath;
    static const std::string s_FragmentShaderPath;
};