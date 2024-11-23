#pragma once
#include "glad\glad.h"
#include <imgui.h>
#include "../Geometry/Geometry.h"
#include "../Geometry/GeometryRenderer.h"

class GeometryViewerView {
public:
    static GeometryViewerView& Get() {
        static GeometryViewerView instance;
        return instance;
    }

    void SetUpViewport();
    void HandleInput();
    void SetGeometry(drosim::editor::Geometry* geometry) {
        m_initialized = true;
        m_geometryBuffers = GeometryRenderer::CreateBuffersFromGeometry(geometry);
    };

    void DrawControls() {
        ImGui::Begin("Geometry Viewer Controls");

        if (m_geometryBuffers) {
            for (const auto& lod : m_geometryBuffers->lodLevels) {
                if (ImGui::TreeNode(lod->name.c_str())) {
                    ImGui::Text("Threshold: %.2f", lod->threshold);
                    ImGui::Text("Meshes Count: %zu", lod->meshBuffers.size());
                    ImGui::TreePop();
                }
            }

            ImGui::SliderFloat("Camera Distance", &m_cameraDistance, 0.0f, 100.0f);
        }

        ImGui::End();
    }
    glm::mat4 CalculateViewMatrix();

    void Draw();
    void EnsureFramebuffer(float width, float height);

private:
    bool m_initialized = false;
    // OpenGL rendering resources
    GLuint m_framebuffer = 0;
    GLuint m_colorTexture = 0;
    GLuint m_depthTexture = 0;
    
    // Basic camera controls
    float m_cameraDistance = 10.0f;
    float m_cameraPosition[3] = {0.0f, 0.0f, -5.0f};
    float m_cameraRotation[2] = {0.0f, 0.0f}; // pitch, yaw
    
    std::unique_ptr<GeometryRenderer::LODGroupBuffers> m_geometryBuffers;
    std::shared_ptr<drosim::editor::Geometry> m_currentGeometry;
};