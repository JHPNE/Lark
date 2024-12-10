#pragma once
#include "../Geometry/Geometry.h"
#include "../Geometry/GeometryRenderer.h"
#include "Components/Geometry.h"
#include "Project/Project.h"
#include "Utils/Utils.h"
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
    uint32_t entity_id{static_cast<uint32_t>(Utils::INVALIDID)};
    bool visible{true};
};

class GeometryViewerView {

public:
    static GeometryViewerView& Get() {
        static GeometryViewerView instance;
        return instance;
    }

    ~GeometryViewerView();

    void SetUpViewport();
    void AddGeometry(uint32_t id);
    void UpdateGeometry(uint32_t id);
    void RemoveGeometry(uint32_t id) {
        auto it = m_geometries.find(id);
        if (it != m_geometries.end()) {
            if (m_selectedGeometry == it->second.get()) {
                m_selectedGeometry = nullptr;
            }
            m_geometries.erase(it);
        }
    }
    void HandleInput();
    void DrawControls();

    void Draw();
    void EnsureFramebuffer(float width, float height);
    void SetActiveProject(std::shared_ptr<Project> activeProject);
    void LoadExistingGeometry();

    void ClearGeometries() {
        // Clear the map which will trigger destructors for ViewportGeometry
        // and its contained buffers, cleaning up OpenGL resources
        m_geometries.clear();
        m_selectedGeometry = nullptr;
        m_loaded = false;
    }

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

    bool GetGeometryTransform(ViewportGeometry* geom, transform_component& transform);

    drosim::editor::scene* GetScene(std::shared_ptr<Project> project, uint32_t id) {
        if (!project) {
            printf("[AddGeometry] No active project.\n");
            return nullptr;
        }

        // Check if entity still exists in active scene
        auto activeScene = project->GetActiveScene();
        if (!activeScene) {
            printf("[AddGeometry] No active scene.\n");
            return nullptr;
        }

        auto entity = activeScene->GetEntity(id);
        if (!entity) {
            printf("[AddGeometry] Entity %u not found in active scene.\n", id);
            return nullptr;
        }

        auto geometryComponent = entity->GetComponent<Geometry>();
        if (!geometryComponent) {
            printf("[AddGeometry] Entity %u has no Geometry component.\n", id);
            return nullptr;
        }

        auto scene = geometryComponent->GetScene();
        if (!scene) {
            printf("[AddGeometry] Entity %u has no LOD group.\n", id);
            return nullptr;
        }
        return scene;
    }



    bool m_loaded = false;
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

    std::unordered_map<uint32_t, std::unique_ptr<ViewportGeometry>> m_geometries;
    std::unique_ptr<GeometryRenderer::LODGroupBuffers> m_geometryBuffers;
    std::shared_ptr<drosim::editor::Geometry> m_currentGeometry;
    ViewportGeometry* m_selectedGeometry{nullptr};  // Track selected geometry
    ImGuizmo::OPERATION m_guizmoOperation{ImGuizmo::OPERATION::TRANSLATE}; // Guizmo operation
    bool m_isUsingGuizmo{false}; // Is guizmo being used?
    float m_guizmoMatrix[4][4]{0.0f}; // Guizmo matrix

    std::shared_ptr<Project> project;
};