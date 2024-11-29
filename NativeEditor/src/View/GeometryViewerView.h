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
    glm::vec3 position{0.0f};
    glm::vec3 rotation{0.0f};
    glm::vec3 scale{1.0f};
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
    bool m_initialized = false;
    // OpenGL rendering resources
    GLuint m_framebuffer = 0;
    GLuint m_colorTexture = 0;
    GLuint m_depthTexture = 0;

    void ResetCamera();
    void ResetGeometryTransform(ViewportGeometry* geom);

    // Basic camera controls
    float m_cameraDistance = 10.0f;
    float m_cameraPosition[3] = {0.0f, 0.0f, -5.0f};
    float m_cameraRotation[2] = {0.0f, 0.0f}; // pitch, yaw

    std::unordered_map<std::string, std::unique_ptr<ViewportGeometry>> m_geometries;
    std::unique_ptr<GeometryRenderer::LODGroupBuffers> m_geometryBuffers;
    std::shared_ptr<drosim::editor::Geometry> m_currentGeometry;
    ViewportGeometry* m_selectedGeometry{nullptr};  // Track selected geometry
    ImGuizmo::OPERATION m_guizmoOperation{ImGuizmo::OPERATION::TRANSLATE}; // Guizmo operation
    bool m_isUsingGuizmo{false}; // Is guizmo being used?
    float m_guizmoMatrix[4][4]{0.0f}; // Guizmo matrix
};