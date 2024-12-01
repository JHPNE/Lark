#pragma once
#include "../Geometry/Geometry.h"
#include "../Geometry/GeometryRenderer.h"
#include "glad/glad.h"
#include <imgui.h>

#include <ImGuizmo.h>
#include <glm/glm.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include <glm/gtx/quaternion.hpp>
#include <string>
#include <unordered_map>

struct ViewportGeometry {
    std::string name;
    std::unique_ptr<GeometryRenderer::LODGroupBuffers> buffers;
    uint32_t entity_id;
    bool visible{true};
};
class GeometryViewerView {
public:
    static GeometryViewerView& Get() {
        static GeometryViewerView instance;
        return instance;
    }

    void SetUpViewport();

    void AddGeometry(const std::string& name, drosim::editor::Geometry* geometry);
    void RemoveGeometry(const std::string& name);
    void HandleInput();
    void SetGeometry(drosim::editor::Geometry* geometry) {
        m_initialized = true;
        m_geometryBuffers = GeometryRenderer::CreateBuffersFromGeometry(geometry);
    };

    void DrawControls();
    glm::mat4 CalculateViewMatrix();

    void Draw();
    void EnsureFramebuffer(float width, float height);

private:
    void UpdateTransformFromGuizmo(ViewportGeometry* geometry, const float* matrix) {
        if (!geometry) return;

        // Decompose the matrix into transform components
        glm::mat4 transform = glm::make_mat4(matrix);
        glm::vec3 position, scale, skew;
        glm::vec4 perspective;
        glm::quat rotation;

        if (glm::decompose(transform, scale, rotation, position, skew, perspective)) {
            transform_component transform_data{};

            // Fill position
            transform_data.position[0] = position.x;
            transform_data.position[1] = position.y;
            transform_data.position[2] = position.z;

            // Convert quaternion to euler angles
            glm::vec3 euler = glm::degrees(glm::eulerAngles(rotation));
            transform_data.rotation[0] = euler.x;
            transform_data.rotation[1] = euler.y;
            transform_data.rotation[2] = euler.z;

            // Fill scale
            transform_data.scale[0] = scale.x;
            transform_data.scale[1] = scale.y;
            transform_data.scale[2] = scale.z;

            // Update the engine transform
            SetEntityTransform(geometry->entity_id, transform_data);
        }
    }

    void CreateEntityForGeometry(ViewportGeometry* geometry) {
        // Create default transform data
        transform_component transform{};
        transform.position[0] = transform.position[1] = transform.position[2] = 0.0f;
        transform.rotation[0] = transform.rotation[1] = transform.rotation[2] = 0.0f;
        transform.scale[0] = transform.scale[1] = transform.scale[2] = 1.0f;

        // Create entity descriptor
        game_entity_descriptor desc{};
        desc.transform = transform;

        // Create the entity
        geometry->entity_id = CreateGameEntity(&desc);
    }

    bool m_initialized = false;
    // OpenGL rendering resources
    GLuint m_framebuffer = 0;
    GLuint m_colorTexture = 0;
    GLuint m_depthTexture = 0;

    void ResetCamera();
    void ResetGeometryTransform(ViewportGeometry* geom);

    // Basic camera controls
    float m_cameraDistance = 10.0f;
    float m_cameraPosition[3] = {0.0f, 0.0f, 0.0f};
    float m_cameraRotation[3] = {0.0f, 0.0f, 0.0f}; // pitch, yaw, roll

    std::unordered_map<std::string, std::unique_ptr<ViewportGeometry>> m_geometries;
    std::unique_ptr<GeometryRenderer::LODGroupBuffers> m_geometryBuffers;
    std::shared_ptr<drosim::editor::Geometry> m_currentGeometry;
    ViewportGeometry* m_selectedGeometry{nullptr};  // Track selected geometry
    ImGuizmo::OPERATION m_guizmoOperation{ImGuizmo::OPERATION::TRANSLATE}; // Guizmo operation
    bool m_isUsingGuizmo{false}; // Is guizmo being used?
    float m_guizmoMatrix[4][4]{0.0f}; // Guizmo matrix
};