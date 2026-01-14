// RaytracingRendererSSBO.h (for Windows/Linux)
#pragma once
#include "RaytracingRendererBase.h"
#include "glad/glad.h"

class RaytracingRendererSSBO : public RaytracingRendererBase
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

private:
    bool CreateShaders();
    bool CreateFullscreenQuad();
    void CreateSSBOs();

    GLuint m_ShaderProgram = 0;
    GLuint m_QuadVAO = 0;
    GLuint m_QuadVBO = 0;
    GLuint m_TriangleSSBO = 0;
    GLuint m_MaterialSSBO = 0;

    int m_TriangleCount = 0;
    int m_MaterialCount = 0;
    bool m_Initialized = false;

    static const std::string s_VertexShaderPath;
    static const std::string s_FragmentShaderPath;
};