#include "RaytracingRendererTBO.h"
#include "Utils/Etc/ShaderParser.h"
#include <fstream>
#include <sstream>
#include <iostream>

// Shader paths - adjust these paths as needed
const std::string RaytracingRendererTBO::s_VertexShaderPath =
    "/Users/am/CLionProjects/Lark/LarkEditor/src/Rendering/Raytracing/Shader/raytracing_mac.vert";
const std::string RaytracingRendererTBO::s_FragmentShaderPath =
    "/Users/am/CLionProjects/Lark/LarkEditor/src/Rendering/Raytracing/Shader/raytracing_mac.frag";

bool RaytracingRendererTBO::Initialize()
{
    if (m_Initialized)
    {
        printf("[RaytracingRendererTBO] Already initialized\n");
        return true;
    }

    printf("[RaytracingRendererTBO] Initializing with Texture Buffer Objects...\n");

    // Query TBO limits
    GLint maxTBOSize;
    glGetIntegerv(GL_MAX_TEXTURE_BUFFER_SIZE, &maxTBOSize);
    printf("[RaytracingRendererTBO] Max TBO size: %d texels (%.2f MB)\n",
           maxTBOSize, (maxTBOSize * 16.0f) / (1024.0f * 1024.0f));

    if (!CreateShaders())
    {
        printf("[RaytracingRendererTBO] Failed to create shaders\n");
        return false;
    }

    if (!CreateFullscreenQuad())
    {
        printf("[RaytracingRendererTBO] Failed to create fullscreen quad\n");
        return false;
    }

    CreateTBOs();

    m_Initialized = true;
    printf("[RaytracingRendererTBO] Initialized successfully\n");
    return true;
}

void RaytracingRendererTBO::Shutdown()
{
    if (!m_Initialized)
        return;

    printf("[RaytracingRendererTBO] Shutting down...\n");

    if (m_ShaderProgram)
    {
        glDeleteProgram(m_ShaderProgram);
        m_ShaderProgram = 0;
    }

    if (m_QuadVAO)
    {
        glDeleteVertexArrays(1, &m_QuadVAO);
        m_QuadVAO = 0;
    }

    if (m_QuadVBO)
    {
        glDeleteBuffers(1, &m_QuadVBO);
        m_QuadVBO = 0;
    }

    // Delete triangle TBO resources
    if (m_TriangleTBO)
    {
        glDeleteTextures(1, &m_TriangleTBO);
        m_TriangleTBO = 0;
    }
    if (m_TriangleBuffer)
    {
        glDeleteBuffers(1, &m_TriangleBuffer);
        m_TriangleBuffer = 0;
    }

    // Delete material TBO resources
    if (m_MaterialTBO)
    {
        glDeleteTextures(1, &m_MaterialTBO);
        m_MaterialTBO = 0;
    }
    if (m_MaterialBuffer)
    {
        glDeleteBuffers(1, &m_MaterialBuffer);
        m_MaterialBuffer = 0;
    }

    // Delete light TBO resources
    if (m_LightTBO)
    {
        glDeleteTextures(1, &m_LightTBO);
        m_LightTBO = 0;
    }
    if (m_LightBuffer)
    {
        glDeleteBuffers(1, &m_LightBuffer);
        m_LightBuffer = 0;
    }

    m_TriangleCount = 0;
    m_MaterialCount = 0;
    m_LightCount = 0;
    m_Initialized = false;

    printf("[RaytracingRendererTBO] Shutdown complete\n");
}

bool RaytracingRendererTBO::CreateShaders()
{
    m_ShaderProgram = ShaderParser::CreateShaderProgram(s_VertexShaderPath, s_FragmentShaderPath);

    if (m_ShaderProgram == 0)
    {
        printf("[RaytracingRendererTBO] Failed to create shader program\n");
        return false;
    }

    printf("[RaytracingRendererTBO] Shaders created successfully\n");
    return true;
}

bool RaytracingRendererTBO::CreateFullscreenQuad()
{
    float quadVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };

    glGenVertexArrays(1, &m_QuadVAO);
    glGenBuffers(1, &m_QuadVBO);

    glBindVertexArray(m_QuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_QuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

    glBindVertexArray(0);

    printf("[RaytracingRendererTBO] Fullscreen quad created\n");
    return true;
}

void RaytracingRendererTBO::CreateTBOs()
{
    // Create triangle TBO
    glGenBuffers(1, &m_TriangleBuffer);
    glGenTextures(1, &m_TriangleTBO);

    // Create material TBO
    glGenBuffers(1, &m_MaterialBuffer);
    glGenTextures(1, &m_MaterialTBO);

    // Create light TBO
    glGenBuffers(1, &m_LightBuffer);
    glGenTextures(1, &m_LightTBO);

    printf("[RaytracingRendererTBO] TBOs created\n");
}

std::vector<TriangleTBOData> RaytracingRendererTBO::PackTrianglesForTBO(const std::vector<Triangle>& triangles)
{
    std::vector<TriangleTBOData> tboData;
    tboData.reserve(triangles.size());

    for (const auto& tri : triangles)
    {
        TriangleTBOData data;
        data.v0 = tri.v0;
        data.v1 = tri.v1;
        data.v2 = tri.v2;
        data.n0 = tri.n0;
        data.n1 = tri.n1;
        data.n2 = tri.n2;
        data.uvData0 = glm::vec4(tri.uv0, tri.uv1);
        data.uvData1 = glm::vec4(tri.uv2, glm::uintBitsToFloat(tri.materialId), 0.0f);
        tboData.push_back(data);
    }

    return tboData;
}

void RaytracingRendererTBO::UploadScene(const RayTracingScene& scene)
{
    if (!m_Initialized)
    {
        printf("[RaytracingRendererTBO] Cannot upload scene - renderer not initialized\n");
        return;
    }

    // Upload triangles
    m_TriangleCount = static_cast<int>(scene.triangles.size());
    if (m_TriangleCount > 0)
    {
        auto tboData = PackTrianglesForTBO(scene.triangles);

        glBindBuffer(GL_TEXTURE_BUFFER, m_TriangleBuffer);
        glBufferData(GL_TEXTURE_BUFFER,
                     tboData.size() * sizeof(TriangleTBOData),
                     tboData.data(),
                     GL_DYNAMIC_DRAW);

        glBindTexture(GL_TEXTURE_BUFFER, m_TriangleTBO);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_TriangleBuffer);

        glBindTexture(GL_TEXTURE_BUFFER, 0);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);
    }

    // Upload materials
    m_MaterialCount = static_cast<int>(scene.materials.size());
    if (m_MaterialCount > 0)
    {
        glBindBuffer(GL_TEXTURE_BUFFER, m_MaterialBuffer);
        glBufferData(GL_TEXTURE_BUFFER,
                     scene.materials.size() * sizeof(PBRMaterial),
                     scene.materials.data(),
                     GL_DYNAMIC_DRAW);

        glBindTexture(GL_TEXTURE_BUFFER, m_MaterialTBO);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_MaterialBuffer);

        glBindTexture(GL_TEXTURE_BUFFER, 0);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);
    }

    // Upload lights
    m_LightCount = static_cast<int>(scene.lights.size());
    if (m_LightCount > 0)
    {
        glBindBuffer(GL_TEXTURE_BUFFER, m_LightBuffer);
        glBufferData(GL_TEXTURE_BUFFER,
                     scene.lights.size() * sizeof(RaytracingLight),
                     scene.lights.data(),
                     GL_DYNAMIC_DRAW);

        glBindTexture(GL_TEXTURE_BUFFER, m_LightTBO);
        glTexBuffer(GL_TEXTURE_BUFFER, GL_RGBA32F, m_LightBuffer);

        glBindTexture(GL_TEXTURE_BUFFER, 0);
        glBindBuffer(GL_TEXTURE_BUFFER, 0);
    }

    printf("[RaytracingRendererTBO] Scene uploaded: %d triangles, %d materials, %d lights\n",
           m_TriangleCount, m_MaterialCount, m_LightCount);
}

void RaytracingRendererTBO::Render(
    const glm::vec3& cameraPos,
    const glm::vec3& cameraFront,
    const glm::vec3& cameraUp,
    float fov,
    float aspectRatio,
    int viewportWidth,
    int viewportHeight)
{
    if (!m_Initialized || !m_ShaderProgram)
    {
        printf("[RaytracingRendererTBO] Cannot render - not initialized\n");
        return;
    }

    glUseProgram(m_ShaderProgram);

    // Calculate camera right vector
    glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, cameraUp));

    // Set camera uniforms
    glUniform3fv(glGetUniformLocation(m_ShaderProgram, "u_CameraPos"), 1, glm::value_ptr(cameraPos));
    glUniform3fv(glGetUniformLocation(m_ShaderProgram, "u_CameraFront"), 1, glm::value_ptr(cameraFront));
    glUniform3fv(glGetUniformLocation(m_ShaderProgram, "u_CameraUp"), 1, glm::value_ptr(cameraUp));
    glUniform3fv(glGetUniformLocation(m_ShaderProgram, "u_CameraRight"), 1, glm::value_ptr(cameraRight));
    glUniform1f(glGetUniformLocation(m_ShaderProgram, "u_Fov"), fov);
    glUniform1f(glGetUniformLocation(m_ShaderProgram, "u_AspectRatio"), aspectRatio);

    // Set resolution
    glUniform2f(glGetUniformLocation(m_ShaderProgram, "u_Resolution"),
                static_cast<float>(viewportWidth),
                static_cast<float>(viewportHeight));

    // Set scene counts
    glUniform1i(glGetUniformLocation(m_ShaderProgram, "u_TriangleCount"), m_TriangleCount);
    glUniform1i(glGetUniformLocation(m_ShaderProgram, "u_MaterialCount"), m_MaterialCount);
    glUniform1i(glGetUniformLocation(m_ShaderProgram, "u_LightCount"), m_LightCount);

    // Bind TBOs to texture units
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, m_TriangleTBO);
    glUniform1i(glGetUniformLocation(m_ShaderProgram, "u_TriangleData"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, m_MaterialTBO);
    glUniform1i(glGetUniformLocation(m_ShaderProgram, "u_MaterialData"), 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_BUFFER, m_LightTBO);
    glUniform1i(glGetUniformLocation(m_ShaderProgram, "u_LightData"), 2);

    // Draw fullscreen quad
    glBindVertexArray(m_QuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    // Unbind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_BUFFER, 0);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_BUFFER, 0);

    glUseProgram(0);
}
