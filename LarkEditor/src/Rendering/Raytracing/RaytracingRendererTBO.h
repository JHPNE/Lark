// RaytracingRendererTBO.h (for macOS)
#pragma once
#include "RaytracingRendererBase.h"
#include "glad/glad.h"

// Data structure for packing triangle data into TBO
struct TriangleTBOData
{
    glm::vec4 v0;  // xyz = position, w unused
    glm::vec4 v1;
    glm::vec4 v2;
    glm::vec4 n0;  // xyz = normal, w unused
    glm::vec4 n1;
    glm::vec4 n2;
    glm::vec4 uvData0;  // xy = uv0, zw = uv1
    glm::vec4 uvData1;  // xy = uv2, z = materialId (as float), w unused
};

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
    int GetLightCount() const override { return m_LightCount; }

private:
    bool CreateShaders();
    bool CreateFullscreenQuad();
    void CreateTBOs();

    std::vector<TriangleTBOData> PackTrianglesForTBO(const std::vector<Triangle>& triangles);

    GLuint m_ShaderProgram = 0;
    GLuint m_QuadVAO = 0;
    GLuint m_QuadVBO = 0;

    // TBO resources
    GLuint m_TriangleTBO = 0;
    GLuint m_TriangleBuffer = 0;
    GLuint m_MaterialTBO = 0;
    GLuint m_MaterialBuffer = 0;
    GLuint m_LightTBO = 0;
    GLuint m_LightBuffer = 0;

    int m_TriangleCount = 0;
    int m_MaterialCount = 0;
    int m_LightCount = 0;
    bool m_Initialized = false;

    static const std::string s_VertexShaderPath;
    static const std::string s_FragmentShaderPath;
};