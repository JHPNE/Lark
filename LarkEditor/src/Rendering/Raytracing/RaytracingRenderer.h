#pragma once
#include <vector>
#include <memory>
#include <string>
#include "GeometryConverter.h"
#include "RaytracingLight.h"
#include "../Materials/PBRMaterial.h"
#include "glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct RayTracingScene
{
    std::vector<Triangle> triangles;
    std::vector<PBRMaterial> materials;
    std::vector<RaytracingLight> lights;
    
    // Scene settings
    glm::vec3 backgroundColor = glm::vec3(0.1f, 0.1f, 0.2f);
    glm::vec3 ambientColor = glm::vec3(0.1f, 0.1f, 0.15f);
};

/**
 * @brief GPU Raytracing Renderer using fragment shader
 * 
 * Renders scenes using raytracing via fullscreen quad fragment shader.
 * Uses SSBOs for triangle and material data.
 */
class RaytracingRenderer
{
public:
    /**
     * @brief Initialize the raytracing renderer
     * @return true if initialization was successful
     */
    static bool Initialize();
    
    /**
     * @brief Shutdown and clean up resources
     */
    static void Shutdown();
    
    /**
     * @brief Check if the renderer is initialized
     */
    static bool IsInitialized() { return s_Initialized; }
    
    /**
     * @brief Upload scene data to GPU
     * @param scene The scene data to upload
     */
    static void UploadScene(const RayTracingScene& scene);
    
    /**
     * @brief Add geometry to existing scene
     * @param scene Scene to add geometry to
     * @param geometry Geometry to add
     * @param transform Transform matrix for the geometry
     * @param materialId Material ID for the triangles
     */
    static void AddGeometryToScene(
        RayTracingScene& scene,
        const content_tools::scene* geometry,
        const glm::mat4& transform = glm::mat4(1.0f),
        uint32_t materialId = 0);
    
    /**
     * @brief Add a PBR material to the scene
     * @param scene Scene to add to
     * @param material The PBR material
     * @return Material index
     */
    static uint32_t AddMaterial(RayTracingScene& scene, const PBRMaterial& material);
    
    /**
     * @brief Add a light to the scene
     * @param scene Scene to add to
     * @param light The light to add
     */
    static void AddLight(RayTracingScene& scene, const RaytracingLight& light);
    
    /**
     * @brief Render the scene using raytracing
     * @param cameraPos Camera position
     * @param cameraFront Camera forward direction
     * @param cameraUp Camera up vector
     * @param fov Field of view in degrees
     * @param aspectRatio Viewport aspect ratio
     * @param viewportWidth Viewport width
     * @param viewportHeight Viewport height
     */
    static void Render(
        const glm::vec3& cameraPos,
        const glm::vec3& cameraFront,
        const glm::vec3& cameraUp,
        float fov,
        float aspectRatio,
        int viewportWidth,
        int viewportHeight);
    
    /**
     * @brief Get current triangle count
     */
    static int GetTriangleCount() { return s_TriangleCount; }
    
    /**
     * @brief Get current material count
     */
    static int GetMaterialCount() { return s_MaterialCount; }

private:
    static bool CreateShaders();
    static bool CreateFullscreenQuad();
    static void CreateSSBOs();

    // Shader program
    static GLuint s_ShaderProgram;
    
    // Fullscreen quad VAO/VBO
    static GLuint s_QuadVAO;
    static GLuint s_QuadVBO;
    
    // Shader Storage Buffer Objects
    static GLuint s_TriangleSSBO;
    static GLuint s_MaterialSSBO;
    static GLuint s_LightSSBO;
    
    // Scene data counts
    static int s_TriangleCount;
    static int s_MaterialCount;
    static int s_LightCount;
    
    // State
    static bool s_Initialized;
    
    // Shader paths
    static const std::string s_VertexShaderPath;
    static const std::string s_FragmentShaderPath;
};