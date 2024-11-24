#include "GeometryViewerView.h"

void GeometryViewerView::Draw() {
    if (!m_initialized) return;

    ImGui::PushID("GeometryViewerMain");
    if (ImGui::Begin("Geometry Viewer##Main")) {
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();

        if (viewportSize.x > 0 && viewportSize.y > 0) {
            SetUpViewport();

            // Create rotation matrix
            glm::mat4 rotation = glm::mat4(1.0f);
            rotation = glm::rotate(rotation, glm::radians(m_rotation[0]), glm::vec3(1.0f, 0.0f, 0.0f));
            rotation = glm::rotate(rotation, glm::radians(m_rotation[1]), glm::vec3(0.0f, 1.0f, 0.0f));
            rotation = glm::rotate(rotation, glm::radians(m_rotation[2]), glm::vec3(0.0f, 0.0f, 1.0f));

            // Create view matrix with position and rotation
            glm::mat4 view = glm::lookAt(
                glm::vec3(m_position[0], m_position[1], m_position[2] - m_cameraDistance),
                glm::vec3(m_position[0], m_position[1], m_position[2]),
                glm::vec3(0.0f, 1.0f, 0.0f)
            );
            view = view * rotation;

            float aspectRatio = viewportSize.x / viewportSize.y;
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

            if (m_geometryBuffers) {
                GeometryRenderer::RenderGeometryAtLOD(m_geometryBuffers.get(), view, projection, m_cameraDistance);
            }

            if (m_colorTexture) {
                ImGui::Image(reinterpret_cast<ImTextureID>((void*)(uintptr_t)m_colorTexture), viewportSize);
            }
        }
    }
    ImGui::End();
    ImGui::PopID();

    DrawControls();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDisable(GL_DEPTH_TEST);
}

void GeometryViewerView::DrawControls() {
    ImGui::PushID("GeometryViewerControls");
    if (ImGui::Begin("Geometry Controls##ViewerControls")) {
        if (m_geometryBuffers) {
            // Camera controls group
            if (ImGui::CollapsingHeader("Camera Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
                // Position controls
                ImGui::Text("Position:");
                ImGui::PushID("Position");
                ImGui::DragFloat("X", &m_position[0], 0.1f);
                ImGui::DragFloat("Y", &m_position[1], 0.1f);
                ImGui::DragFloat("Z", &m_position[2], 0.1f);
                if (ImGui::Button("Reset Position")) {
                    m_position[0] = m_position[1] = m_position[2] = 0.0f;
                }
                ImGui::PopID();

                ImGui::Separator();

                // Rotation controls
                ImGui::Text("Rotation:");
                ImGui::PushID("Rotation");
                ImGui::DragFloat("Pitch", &m_rotation[0], 1.0f, -180.0f, 180.0f);
                ImGui::DragFloat("Yaw", &m_rotation[1], 1.0f, -180.0f, 180.0f);
                ImGui::DragFloat("Roll", &m_rotation[2], 1.0f, -180.0f, 180.0f);
                if (ImGui::Button("Reset Rotation")) {
                    m_rotation[0] = m_rotation[1] = m_rotation[2] = 0.0f;
                }
                ImGui::PopID();

                ImGui::Separator();

                // Camera distance
                ImGui::DragFloat("Camera Distance", &m_cameraDistance, 0.1f, 0.1f, 100.0f);
                if (ImGui::Button("Reset Camera")) {
                    m_cameraDistance = 10.0f;
                }
            }

            // LOD information
            if (ImGui::CollapsingHeader("LOD Information")) {
                ImGui::PushID("LODControls");
                for (size_t i = 0; i < m_geometryBuffers->lodLevels.size(); ++i) {
                    const auto& lod = m_geometryBuffers->lodLevels[i];
                    if (ImGui::TreeNode((void*)(intptr_t)i, "%s", lod->name.c_str())) {
                        ImGui::Text("Threshold: %.2f", lod->threshold);
                        ImGui::Text("Meshes Count: %zu", lod->meshBuffers.size());
                        ImGui::TreePop();
                    }
                }
                ImGui::PopID();
            }
        }
        else {
            ImGui::TextDisabled("No geometry loaded");
        }
    }
    ImGui::End();
    ImGui::PopID();
}

void GeometryViewerView::SetUpViewport() {
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();

    if (viewportSize.x <= 0 || viewportSize.y <= 0) {
        return;  // Skip if invalid size
    }

    // Create or resize framebuffer if needed
    EnsureFramebuffer(viewportSize.x, viewportSize.y);

    // Bind framebuffer and set viewport
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glViewport(0, 0, (GLsizei)viewportSize.x, (GLsizei)viewportSize.y);

    // Clear buffers
    glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
}

void GeometryViewerView::EnsureFramebuffer(float width, float height) {
    // If framebuffer already exists with correct size, return
    if (m_framebuffer && m_colorTexture && m_depthTexture) {
        GLint texWidth, texHeight;
        glBindTexture(GL_TEXTURE_2D, m_colorTexture);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texWidth);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texHeight);
        
        if (texWidth == (GLint)width && texHeight == (GLint)height) {
            return;
        }
    }
    
    // Clean up existing framebuffer if it exists
    if (m_framebuffer) {
        glDeleteFramebuffers(1, &m_framebuffer);
        glDeleteTextures(1, &m_colorTexture);
        glDeleteTextures(1, &m_depthTexture);
    }
    
    // Create framebuffer
    glGenFramebuffers(1, &m_framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    
    // Create color attachment texture
    glGenTextures(1, &m_colorTexture);
    glBindTexture(GL_TEXTURE_2D, m_colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);
    
    // Create depth texture
    glGenTextures(1, &m_depthTexture);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, (GLsizei)width, (GLsizei)height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);
    
    // Check framebuffer completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        // Handle error
    }
}