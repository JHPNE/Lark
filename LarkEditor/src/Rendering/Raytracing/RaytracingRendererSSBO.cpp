#include "RaytracingRendererSSBO.h"
#include "Utils/Etc/ShaderParser.h"
#include <iostream>

// Shader paths for Windows/Linux SSBO version
const std::string RaytracingRendererSSBO::s_VertexShaderPath =
    "/Users/am/CLionProjects/Lark/LarkEditor/src/Rendering/Raytracing/Shader/raytracing.vert";
const std::string RaytracingRendererSSBO::s_FragmentShaderPath =
    "/Users/am/CLionProjects/Lark/LarkEditor/src/Rendering/Raytracing/Shader/raytracing.frag";

bool RaytracingRendererSSBO::Initialize()
{
    if (m_Initialized)
    {
        printf("[RaytracingRendererSSBO] Already initialized\n");
        return true;
    }
    
    printf("[RaytracingRendererSSBO] Initializing with Shader Storage Buffer Objects...\n");
    
    // Query SSBO limits
    GLint maxSSBOSize;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, &maxSSBOSize);
    printf("[RaytracingRendererSSBO] Max SSBO size: %d bytes (%.2f MB)\n", 
           maxSSBOSize, maxSSBOSize / (1024.0f * 1024.0f));
    
    if (!CreateShaders())
    {
        printf("[RaytracingRendererSSBO] Failed to create shaders\n");
        return false;
    }
    
    if (!CreateFullscreenQuad())
    {
        printf("[RaytracingRendererSSBO] Failed to create fullscreen quad\n");
        return false;
    }
    
    CreateSSBOs();
    
    m_Initialized = true;
    printf("[RaytracingRendererSSBO] Initialized successfully\n");
    return true;
}

void RaytracingRendererSSBO::Shutdown()
{
    if (!m_Initialized)
        return;
    
    printf("[RaytracingRendererSSBO] Shutting down...\n");
    
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
    
    if (m_TriangleSSBO)
    {
        glDeleteBuffers(1, &m_TriangleSSBO);
        m_TriangleSSBO = 0;
    }
    
    if (m_MaterialSSBO)
    {
        glDeleteBuffers(1, &m_MaterialSSBO);
        m_MaterialSSBO = 0;
    }
    
    if (m_LightSSBO)
    {
        glDeleteBuffers(1, &m_LightSSBO);
        m_LightSSBO = 0;
    }
    
    m_TriangleCount = 0;
    m_MaterialCount = 0;
    m_LightCount = 0;
    m_Initialized = false;
    
    printf("[RaytracingRendererSSBO] Shutdown complete\n");
}

bool RaytracingRendererSSBO::CreateShaders()
{
    m_ShaderProgram = ShaderParser::CreateShaderProgram(s_VertexShaderPath, s_FragmentShaderPath);
    
    if (m_ShaderProgram == 0)
    {
        printf("[RaytracingRendererSSBO] Failed to create shader program\n");
        return false;
    }
    
    printf("[RaytracingRendererSSBO] Shaders created successfully\n");
    return true;
}

bool RaytracingRendererSSBO::CreateFullscreenQuad()
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
    
    printf("[RaytracingRendererSSBO] Fullscreen quad created\n");
    return true;
}

void RaytracingRendererSSBO::CreateSSBOs()
{
    glGenBuffers(1, &m_TriangleSSBO);
    glGenBuffers(1, &m_MaterialSSBO);
    glGenBuffers(1, &m_LightSSBO);
    
    printf("[RaytracingRendererSSBO] SSBOs created\n");
}

void RaytracingRendererSSBO::UploadScene(const RayTracingScene& scene)
{
    if (!m_Initialized)
    {
        printf("[RaytracingRendererSSBO] Cannot upload scene - renderer not initialized\n");
        return;
    }
    
    // Upload triangles
    m_TriangleCount = static_cast<int>(scene.triangles.size());
    if (m_TriangleCount > 0)
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_TriangleSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 
                     scene.triangles.size() * sizeof(Triangle),
                     scene.triangles.data(), 
                     GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_TriangleSSBO);
    }
    
    // Upload materials
    m_MaterialCount = static_cast<int>(scene.materials.size());
    if (m_MaterialCount > 0)
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_MaterialSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     scene.materials.size() * sizeof(PBRMaterial),
                     scene.materials.data(),
                     GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_MaterialSSBO);
    }
    
    // Upload lights
    m_LightCount = static_cast<int>(scene.lights.size());
    if (m_LightCount > 0)
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_LightSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     scene.lights.size() * sizeof(RaytracingLight),
                     scene.lights.data(),
                     GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_LightSSBO);
    }
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    
    printf("[RaytracingRendererSSBO] Scene uploaded: %d triangles, %d materials, %d lights\n",
           m_TriangleCount, m_MaterialCount, m_LightCount);
}

void RaytracingRendererSSBO::Render(
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
        printf("[RaytracingRendererSSBO] Cannot render - not initialized\n");
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
    
    // Draw fullscreen quad
    glBindVertexArray(m_QuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    glUseProgram(0);
}