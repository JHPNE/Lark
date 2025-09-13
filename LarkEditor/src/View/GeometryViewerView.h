#pragma once
#include <memory>
#include <glm/glm.hpp>
#include "glad/glad.h"
#include <imgui.h>

class Project;
class GeometryViewModel;

class GeometryViewerView
{
    public:
    static GeometryViewerView &Get() {
        static GeometryViewerView instance;
        return instance;
    }

    void Draw();
    void SetActiveProject(std::shared_ptr<Project> activeProject);
    ~GeometryViewerView();

private:
    GeometryViewerView();

    void DrawViewport();
    void DrawControls();
    void DrawGizmo(const ImVec2& canvasPos, const ImVec2& canvasSize,
                  const glm::mat4& view, const glm::mat4& projection);

    void EnsureFramebuffer(float width, float height);
    glm::mat4 CalculateViewMatrix();

    bool m_initialized = false;
    bool m_showFileDialog = false;

    // OpenGL resources
    GLuint m_framebuffer = 0;
    GLuint m_colorTexture = 0;
    GLuint m_depthTexture = 0;

    // OpenGL resources for picking
    GLuint m_pickingFramebuffer = 0;
    GLuint m_pickingColorTexture = 0;
    GLuint m_pickingDepthTexture = 0;
    GLuint m_pickingShader = 0;

    std::unique_ptr<GeometryViewModel> m_viewModel;
};