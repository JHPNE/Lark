#include "GeometryViewerView.h"
#include <imgui.h>
#include "glm\glm.hpp"

void GeometryViewerView::Draw() {
  ImGui::Begin("Geometry Viewer");

  ImVec2 viewportSize = ImGui::GetContentRegionAvail();

  EnsureFramebuffer(viewportSize.x, viewportSize.y);

  if (m_currentGeometry) {
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glViewport(0, 0, viewportSize.x, viewportSize.y);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Camera Matrices
    glm::mat4 projection = glm::perspective(45.0f, viewportSize.x / viewportSize.y, 0.1f, 100.0f);
    glm::mat4 view = CalculateViewMatrix();
    glm::mat4 model = glm::mat4(1.0f);

    auto vertexBuffer = GeometryRenderer::CreateBuffersFromGeometry(m_currentGeometry.get());
    GeometryRenderer::RenderGeometry(vertexBuffer, projection, view, model);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    ImGui::Image(reinterpret_cast<ImTextureID>((void*)(uintptr_t)m_colorTexture), viewportSize);


    HandleInput();
  }

  ImGui::End();
}

void GeometryViewerView::HandleInput() {}

void GeometryViewerView::EnsureFramebuffer(float width, float height) {

}

glm::mat4 GeometryViewerView::CalculateViewMatrix() {}