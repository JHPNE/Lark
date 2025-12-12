#include "RaytracingRenderer.h"

#include "Utils/Etc/ShaderParser.h"

#include <fstream>
#include <sstream>
#include <iostream>

// Static member initialization
GLuint RaytracingRenderer::s_ShaderProgram = 0;
GLuint RaytracingRenderer::s_QuadVAO = 0;
GLuint RaytracingRenderer::s_QuadVBO = 0;
GLuint RaytracingRenderer::s_TriangleSSBO = 0;
GLuint RaytracingRenderer::s_MaterialSSBO = 0;
GLuint RaytracingRenderer::s_LightSSBO = 0;
int RaytracingRenderer::s_TriangleCount = 0;
int RaytracingRenderer::s_MaterialCount = 0;
int RaytracingRenderer::s_LightCount = 0;
bool RaytracingRenderer::s_Initialized = false;

// Shader paths - adjust these paths as needed
const std::string RaytracingRenderer::s_VertexShaderPath = 
    "/Users/am/CLionProjects/Lark/LarkEditor/src/Rendering/Raytracing/Shader/raytracing.vert";
const std::string RaytracingRenderer::s_FragmentShaderPath = 
    "/Users/am/CLionProjects/Lark/LarkEditor/src/Rendering/Raytracing/Shader/raytracing.frag";

bool RaytracingRenderer::Initialize()
{
    if (s_Initialized)
    {
        printf("[RaytracingRenderer] Already initialized\n");
        return true;
    }
    
    printf("[RaytracingRenderer] Initializing...\n");
    
    if (!CreateShaders())
    {
        printf("[RaytracingRenderer] Failed to create shaders\n");
        return false;
    }
    
    if (!CreateFullscreenQuad())
    {
        printf("[RaytracingRenderer] Failed to create fullscreen quad\n");
        return false;
    }
    
    CreateSSBOs();
    
    s_Initialized = true;
    printf("[RaytracingRenderer] Initialized successfully\n");
    return true;
}

void RaytracingRenderer::Shutdown()
{
    if (!s_Initialized)
        return;
    
    printf("[RaytracingRenderer] Shutting down...\n");
    
    if (s_ShaderProgram)
    {
        glDeleteProgram(s_ShaderProgram);
        s_ShaderProgram = 0;
    }
    
    if (s_QuadVAO)
    {
        glDeleteVertexArrays(1, &s_QuadVAO);
        s_QuadVAO = 0;
    }
    
    if (s_QuadVBO)
    {
        glDeleteBuffers(1, &s_QuadVBO);
        s_QuadVBO = 0;
    }
    
    if (s_TriangleSSBO)
    {
        glDeleteBuffers(1, &s_TriangleSSBO);
        s_TriangleSSBO = 0;
    }
    
    if (s_MaterialSSBO)
    {
        glDeleteBuffers(1, &s_MaterialSSBO);
        s_MaterialSSBO = 0;
    }
    
    if (s_LightSSBO)
    {
        glDeleteBuffers(1, &s_LightSSBO);
        s_LightSSBO = 0;
    }
    
    s_TriangleCount = 0;
    s_MaterialCount = 0;
    s_LightCount = 0;
    s_Initialized = false;
    
    printf("[RaytracingRenderer] Shutdown complete\n");
}

bool RaytracingRenderer::CreateShaders()
{
    s_ShaderProgram = ShaderParser::CreateShaderProgram(s_VertexShaderPath, s_FragmentShaderPath);
    return true;
}

bool RaytracingRenderer::CreateFullscreenQuad()
{
    // Fullscreen quad vertices (position only, UVs computed in shader)
    float quadVertices[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f, -1.0f,
         1.0f,  1.0f,
        -1.0f,  1.0f
    };
    
    glGenVertexArrays(1, &s_QuadVAO);
    glGenBuffers(1, &s_QuadVBO);
    
    glBindVertexArray(s_QuadVAO);
    glBindBuffer(GL_ARRAY_BUFFER, s_QuadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    
    glBindVertexArray(0);
    
    printf("[RaytracingRenderer] Fullscreen quad created\n");
    return true;
}

void RaytracingRenderer::CreateSSBOs()
{
    glGenBuffers(1, &s_TriangleSSBO);
    glGenBuffers(1, &s_MaterialSSBO);
    glGenBuffers(1, &s_LightSSBO);
    
    printf("[RaytracingRenderer] SSBOs created\n");
}

void RaytracingRenderer::UploadScene(const RayTracingScene& scene)
{
    if (!s_Initialized)
    {
        printf("[RaytracingRenderer] Cannot upload scene - renderer not initialized\n");
        return;
    }
    
    // Upload triangles
    s_TriangleCount = static_cast<int>(scene.triangles.size());
    if (s_TriangleCount > 0)
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_TriangleSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER, 
                     scene.triangles.size() * sizeof(Triangle),
                     scene.triangles.data(), 
                     GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, s_TriangleSSBO);
    }
    
    // Upload materials
    s_MaterialCount = static_cast<int>(scene.materials.size());
    if (s_MaterialCount > 0)
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_MaterialSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     scene.materials.size() * sizeof(PBRMaterial),
                     scene.materials.data(),
                     GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, s_MaterialSSBO);
    }
    
    // Upload lights
    s_LightCount = static_cast<int>(scene.lights.size());
    if (s_LightCount > 0)
    {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, s_LightSSBO);
        glBufferData(GL_SHADER_STORAGE_BUFFER,
                     scene.lights.size() * sizeof(RaytracingLight),
                     scene.lights.data(),
                     GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, s_LightSSBO);
    }
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    
    printf("[RaytracingRenderer] Scene uploaded: %d triangles, %d materials, %d lights\n",
           s_TriangleCount, s_MaterialCount, s_LightCount);
}

void RaytracingRenderer::AddGeometryToScene(
    RayTracingScene& scene,
    const content_tools::scene* geometry,
    const glm::mat4& transform,
    uint32_t materialId)
{
    if (!geometry) return;
    
    auto triangles = GeometryConverter::ConvertFromGeometry(geometry, materialId);
    
    // Apply transform to triangles
    glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(transform)));
    
    for (auto& tri : triangles)
    {
        // Transform vertices
        glm::vec4 v0 = transform * glm::vec4(tri.v0, 1.0f);
        glm::vec4 v1 = transform * glm::vec4(tri.v1, 1.0f);
        glm::vec4 v2 = transform * glm::vec4(tri.v2, 1.0f);
        
        tri.v0 = glm::vec3(v0);
        tri.v1 = glm::vec3(v1);
        tri.v2 = glm::vec3(v2);
        
        // Transform normals
        tri.n0 = glm::normalize(normalMatrix * tri.n0);
        tri.n1 = glm::normalize(normalMatrix * tri.n1);
        tri.n2 = glm::normalize(normalMatrix * tri.n2);
    }
    
    scene.triangles.insert(scene.triangles.end(), triangles.begin(), triangles.end());
}

uint32_t RaytracingRenderer::AddMaterial(RayTracingScene& scene, const PBRMaterial& material)
{
    uint32_t index = static_cast<uint32_t>(scene.materials.size());
    scene.materials.push_back(material);
    return index;
}

void RaytracingRenderer::AddLight(RayTracingScene& scene, const RaytracingLight& light)
{
    scene.lights.push_back(light);
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
    if (!s_Initialized || !s_ShaderProgram)
    {
        printf("[RaytracingRenderer] Cannot render - not initialized\n");
        return;
    }
    
    glUseProgram(s_ShaderProgram);
    
    // Calculate camera right vector
    glm::vec3 cameraRight = glm::normalize(glm::cross(cameraFront, cameraUp));
    
    // Set camera uniforms
    glUniform3fv(glGetUniformLocation(s_ShaderProgram, "u_CameraPos"), 1, glm::value_ptr(cameraPos));
    glUniform3fv(glGetUniformLocation(s_ShaderProgram, "u_CameraFront"), 1, glm::value_ptr(cameraFront));
    glUniform3fv(glGetUniformLocation(s_ShaderProgram, "u_CameraUp"), 1, glm::value_ptr(cameraUp));
    glUniform3fv(glGetUniformLocation(s_ShaderProgram, "u_CameraRight"), 1, glm::value_ptr(cameraRight));
    glUniform1f(glGetUniformLocation(s_ShaderProgram, "u_Fov"), fov);
    glUniform1f(glGetUniformLocation(s_ShaderProgram, "u_AspectRatio"), aspectRatio);
    
    // Set resolution
    glUniform2f(glGetUniformLocation(s_ShaderProgram, "u_Resolution"), 
                static_cast<float>(viewportWidth), 
                static_cast<float>(viewportHeight));
    
    // Set scene counts
    glUniform1i(glGetUniformLocation(s_ShaderProgram, "u_TriangleCount"), s_TriangleCount);
    glUniform1i(glGetUniformLocation(s_ShaderProgram, "u_MaterialCount"), s_MaterialCount);
    glUniform1i(glGetUniformLocation(s_ShaderProgram, "u_LightCount"), s_LightCount);
    
    // Draw fullscreen quad
    glBindVertexArray(s_QuadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
    
    glUseProgram(0);
}
