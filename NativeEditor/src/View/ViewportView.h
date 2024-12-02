#pragma once

#include "glad/glad.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui.h>
#include <ImGuizmo.h>

namespace drosim::editor {

class ViewportView {
public:
    static ViewportView& Get() {
        static ViewportView instance;
        return instance;
    }

    ~ViewportView() {
        Cleanup();
    }

    void Initialize() {
        if (!m_initialized) {
            m_initialized = true;
            ResetCamera();
        }
    }

    void Cleanup() {
        if (m_framebuffer) {
            glDeleteFramebuffers(1, &m_framebuffer);
            m_framebuffer = 0;
        }
        if (m_colorTexture) {
            glDeleteTextures(1, &m_colorTexture);
            m_colorTexture = 0;
        }
        if (m_depthTexture) {
            glDeleteTextures(1, &m_depthTexture);
            m_depthTexture = 0;
        }
    }

    void Draw() {
        if (!m_initialized) return;

        ImGui::PushID("ViewportMain");
        if (ImGui::Begin("Viewport##Main", nullptr,
            ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {

            ImVec2 viewportSize = ImGui::GetContentRegionAvail();
            if (viewportSize.x > 0 && viewportSize.y > 0) {
                UpdateViewport(viewportSize);
                RenderViewport(viewportSize);
            }
        }
        ImGui::End();
        ImGui::PopID();
    }

private:
    ViewportView() = default;
    ViewportView(const ViewportView&) = delete;
    ViewportView& operator=(const ViewportView&) = delete;

    void UpdateViewport(const ImVec2& size) {
        if (!EnsureFramebuffer(size.x, size.y)) return;

        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
        glViewport(0, 0, (GLsizei)size.x, (GLsizei)size.y);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);

        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    void RenderViewport(const ImVec2& size) {
        if (!m_colorTexture) return;

        glm::mat4 view = CalculateViewMatrix();
        float aspectRatio = size.x / size.y;
        glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);

        // Draw the viewport texture
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
        ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
        ImVec2 canvasPos = ImVec2(windowPos.x + contentMin.x, windowPos.y + contentMin.y);
        ImVec2 canvasSize = ImVec2(contentMax.x - contentMin.x, contentMax.y - contentMin.y);

        ImGui::GetWindowDrawList()->AddImage(
            reinterpret_cast<ImTextureID>((void*)(uintptr_t)m_colorTexture),
            canvasPos,
            ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
            ImVec2(0, 1),
            ImVec2(1, 0)
        );

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    bool EnsureFramebuffer(float width, float height) {
        if (m_framebuffer && m_colorTexture && m_depthTexture) {
            GLint texWidth, texHeight;
            glBindTexture(GL_TEXTURE_2D, m_colorTexture);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texWidth);
            glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texHeight);

            if (texWidth == (GLint)width && texHeight == (GLint)height) {
                return true;
            }
        }

        Cleanup();

        // Create framebuffer
        glGenFramebuffers(1, &m_framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

        // Create color attachment
        glGenTextures(1, &m_colorTexture);
        glBindTexture(GL_TEXTURE_2D, m_colorTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);

        // Create depth attachment
        glGenTextures(1, &m_depthTexture);
        glBindTexture(GL_TEXTURE_2D, m_depthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, (GLsizei)width, (GLsizei)height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            Cleanup();
            return false;
        }

        return true;
    }

    glm::mat4 CalculateViewMatrix() {
        glm::vec3 cameraPos(m_cameraPosition[0], m_cameraPosition[1], m_cameraPosition[2]);
        glm::vec3 forward(0.0f, 0.0f, -1.0f);
        glm::vec3 up(0.0f, 1.0f, 0.0f);

        glm::mat4 rotation = glm::mat4(1.0f);
        rotation = glm::rotate(rotation, glm::radians(m_cameraRotation[0]), glm::vec3(1.0f, 0.0f, 0.0f));
        rotation = glm::rotate(rotation, glm::radians(m_cameraRotation[1]), glm::vec3(0.0f, 1.0f, 0.0f));
        rotation = glm::rotate(rotation, glm::radians(m_cameraRotation[2]), glm::vec3(0.0f, 0.0f, 1.0f));

        forward = glm::vec3(rotation * glm::vec4(forward, 0.0f));
        up = glm::vec3(rotation * glm::vec4(up, 0.0f));

        glm::vec3 actualCameraPos = cameraPos - (forward * m_cameraDistance);
        return glm::lookAt(actualCameraPos, cameraPos, up);
    }

    void ResetCamera() {
        m_cameraPosition[0] = m_cameraPosition[1] = m_cameraPosition[2] = 0.0f;
        m_cameraRotation[0] = m_cameraRotation[1] = m_cameraRotation[2] = 0.0f;
        m_cameraDistance = 10.0f;
    }

    bool m_initialized = false;
    GLuint m_framebuffer = 0;
    GLuint m_colorTexture = 0;
    GLuint m_depthTexture = 0;

    float m_cameraPosition[3] = { 0.0f, 0.0f, 0.0f };
    float m_cameraRotation[3] = { 0.0f, 0.0f, 0.0f };
    float m_cameraDistance = 10.0f;
};

} // namespace drosim::editor