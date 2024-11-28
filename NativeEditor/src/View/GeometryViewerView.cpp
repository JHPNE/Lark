#include "GeometryViewerView.h"

void GeometryViewerView::AddGeometry(const std::string& name, drosim::editor::Geometry* geometry) {
    if (!geometry) return;

    auto buffers = GeometryRenderer::CreateBuffersFromGeometry(geometry);
    if (buffers) {
        m_geometries[name] = std::make_unique<ViewportGeometry>();
        m_geometries[name]->name = name;
        m_geometries[name]->buffers = std::move(buffers);
        m_initialized = true;
    }
}

void GeometryViewerView::RemoveGeometry(const std::string& name) {
    m_geometries.erase(name);
}

void GeometryViewerView::Draw() {
    if (!m_initialized) return;

    ImGui::PushID("GeometryViewerMain");
    if (ImGui::Begin("Geometry Viewer##Main")) {
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();

        if (viewportSize.x > 0 && viewportSize.y > 0) {
            SetUpViewport();

            // Create view matrix with camera position and rotation
            glm::mat4 view = glm::lookAt(
                glm::vec3(m_cameraPosition[0], m_cameraPosition[1], m_cameraPosition[2] - m_cameraDistance),
                glm::vec3(m_cameraPosition[0], m_cameraPosition[1], m_cameraPosition[2]),
                glm::vec3(0.0f, 1.0f, 0.0f)
            );

            // Apply camera rotation
            view = glm::rotate(view, glm::radians(m_cameraRotation[0]), glm::vec3(1.0f, 0.0f, 0.0f));
            view = glm::rotate(view, glm::radians(m_cameraRotation[1]), glm::vec3(0.0f, 1.0f, 0.0f));

            float aspectRatio = viewportSize.x / viewportSize.y;
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);

            // Save OpenGL state
            GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
            GLint last_framebuffer; glGetIntegerv(GL_FRAMEBUFFER_BINDING, &last_framebuffer);

            // Render all visible geometries
            for (const auto& [name, geom] : m_geometries) {
                if (!geom->visible) continue;

                // Create model matrix for this geometry
                glm::mat4 model = glm::mat4(1.0f);
                model = glm::translate(model, geom->position);
                model = glm::rotate(model, glm::radians(geom->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                model = glm::rotate(model, glm::radians(geom->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                model = glm::rotate(model, glm::radians(geom->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
                model = glm::scale(model, geom->scale);

                // Calculate final view matrix including model transform
                glm::mat4 finalView = view * model;

                // Render this geometry
                GeometryRenderer::RenderGeometryAtLOD(
                    geom->buffers.get(),
                    finalView,
                    projection,
                    m_cameraDistance
                );
            }

            // Restore OpenGL state
            glBindFramebuffer(GL_FRAMEBUFFER, last_framebuffer);
            glViewport(last_viewport[0], last_viewport[1], (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);

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
    glDisable(GL_CULL_FACE);
}

void GeometryViewerView::DrawControls() {
    ImGui::PushID("GeometryViewerControls");
    if (ImGui::Begin("Geometry Controls##ViewerControls")) {
        // Camera controls group
        if (ImGui::CollapsingHeader("Camera Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::DragFloat3("Camera Position", m_cameraPosition, 0.1f);
            ImGui::DragFloat2("Camera Rotation", m_cameraRotation, 1.0f);
            ImGui::DragFloat("Camera Distance", &m_cameraDistance, 0.1f, 0.1f, 100.0f);

            if (ImGui::Button("Reset Camera")) {
                ResetCamera();
            }
        }

        // Geometry list and controls
        if (ImGui::CollapsingHeader("Geometries", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (auto& [name, geom] : m_geometries) {
                if (ImGui::TreeNode(name.c_str())) {
                    ImGui::Checkbox("Visible", &geom->visible);

                    ImGui::Text("Position:");
                    ImGui::DragFloat3(("Position##" + name).c_str(), &geom->position.x, 0.1f);

                    ImGui::Text("Rotation:");
                    ImGui::DragFloat3(("Rotation##" + name).c_str(), &geom->rotation.x, 1.0f);

                    ImGui::Text("Scale:");
                    ImGui::DragFloat3(("Scale##" + name).c_str(), &geom->scale.x, 0.1f);

                    if (ImGui::Button(("Reset Transform##" + name).c_str())) {
                        ResetGeometryTransform(geom.get());
                    }

                    ImGui::TreePop();
                }
            }
        }
    }
    ImGui::End();
    ImGui::PopID();
}

void GeometryViewerView::SetUpViewport() {
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    if (viewportSize.x <= 0 || viewportSize.y <= 0) return;

    EnsureFramebuffer(viewportSize.x, viewportSize.y);

    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
    glViewport(0, 0, (GLsizei)viewportSize.x, (GLsizei)viewportSize.y);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
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

void GeometryViewerView::ResetCamera() {
    m_cameraPosition[0] = m_cameraPosition[1] = m_cameraPosition[2] = 0.0f;
    m_cameraRotation[0] = m_cameraRotation[1] = 0.0f;
    m_cameraDistance = 10.0f;
}

void GeometryViewerView::ResetGeometryTransform(ViewportGeometry* geom) {
    if (!geom) return;
    geom->position = glm::vec3(0.0f);
    geom->rotation = glm::vec3(0.0f);
    geom->scale = glm::vec3(1.0f);
}