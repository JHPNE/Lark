#include "GeometryViewerView.h"
#include "../Utils/ImGuizmoManager.h"

void GeometryViewerView::AddGeometry(const std::string& name, drosim::editor::Geometry* geometry) {
    if (!geometry) return;

    auto buffers = GeometryRenderer::CreateBuffersFromGeometry(geometry);
    if (buffers) {
        m_geometries[name] = std::make_unique<ViewportGeometry>();
        m_geometries[name]->name = name;
        m_geometries[name]->buffers = std::move(buffers);
        m_geometries[name]->position = glm::vec3(0.0f);
        m_geometries[name]->rotation = glm::vec3(0.0f);
        m_geometries[name]->scale = glm::vec3(1.0f);
        m_geometries[name]->visible = true;
        m_initialized = true;
        
        // Automatically select the newly added geometry
        m_selectedGeometry = m_geometries[name].get();
    }
}

void GeometryViewerView::RemoveGeometry(const std::string& name) {
    m_geometries.erase(name);
}

void GeometryViewerView::Draw() {
    if (!m_initialized) return;

    ImGui::PushID("GeometryViewerMain");
    if (ImGui::Begin("Geometry Viewer##Main", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
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

                // Handle viewport clicking for selection
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(0) && !ImGuizmo::IsOver()) {
                    // For now, we'll just select the first geometry
                    if (!m_geometries.empty()) {
                        auto it = m_geometries.begin();
                        m_selectedGeometry = it->second.get();
                    }
                }

                // Set up ImGuizmo after the image is drawn
                ImGuizmo::SetOrthographic(false);
                ImGuizmo::SetDrawlist(ImGui::GetWindowDrawList());
                ImGuizmo::SetRect(
                    ImGui::GetWindowPos().x,
                    ImGui::GetWindowPos().y,
                    ImGui::GetWindowWidth(),
                    ImGui::GetWindowHeight()
                );

                // Draw Guizmo for selected geometry
                if (m_selectedGeometry) {
                    // Create model matrix for selected geometry
                    glm::mat4 model = glm::mat4(1.0f);
                    model = glm::translate(model, m_selectedGeometry->position);
                    model = glm::rotate(model, glm::radians(m_selectedGeometry->rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
                    model = glm::rotate(model, glm::radians(m_selectedGeometry->rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
                    model = glm::rotate(model, glm::radians(m_selectedGeometry->rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
                    model = glm::scale(model, m_selectedGeometry->scale);

                    // Convert matrices for ImGuizmo
                    float modelMatrix[16], viewMatrix[16], projMatrix[16];
                    glm::mat4 viewCopy = view;  // Make a copy as Manipulate might modify it
                    glm::mat4 projCopy = projection;

                    // Convert matrices to column-major float arrays for ImGuizmo
                    memcpy(modelMatrix, glm::value_ptr(model), sizeof(float) * 16);
                    memcpy(viewMatrix, glm::value_ptr(viewCopy), sizeof(float) * 16);
                    memcpy(projMatrix, glm::value_ptr(projCopy), sizeof(float) * 16);

                    // Enable operation based on current mode
                    ImGuizmo::OPERATION currentOperation;
                    switch (m_guizmoOperation) {
                        case ImGuizmo::OPERATION::ROTATE:
                            currentOperation = ImGuizmo::OPERATION::ROTATE;
                            break;
                        case ImGuizmo::OPERATION::SCALE:
                            currentOperation = ImGuizmo::OPERATION::SCALE;
                            break;
                        case ImGuizmo::OPERATION::TRANSLATE:
                        default:
                            currentOperation = ImGuizmo::OPERATION::TRANSLATE;
                            break;
                    }

                    // Draw the gizmo
                    bool manipulated = ImGuizmo::Manipulate(
                        viewMatrix,
                        projMatrix,
                        currentOperation,
                        ImGuizmo::MODE::LOCAL,
                        modelMatrix,
                        nullptr,
                        nullptr
                    );

                    // If the matrix was modified, update the geometry transform
                    if (manipulated) {
                        m_isUsingGuizmo = true;
                        
                        // Convert the modified matrix back to glm
                        glm::mat4 newModel = glm::make_mat4(modelMatrix);
                        
                        // Extract position, rotation, and scale
                        glm::vec3 skew;
                        glm::vec4 perspective;
                        glm::quat rotation;
                        
                        // Decompose the new model matrix
                        if (glm::decompose(newModel, m_selectedGeometry->scale, rotation, m_selectedGeometry->position, skew, perspective)) {
                            // Convert quaternion to euler angles (in radians)
                            glm::vec3 rotationRadians = glm::eulerAngles(rotation);
                            
                            // Convert rotation to degrees
                            m_selectedGeometry->rotation = glm::degrees(rotationRadians);
                            
                            // Ensure scale doesn't go negative
                            m_selectedGeometry->scale = glm::abs(m_selectedGeometry->scale);
                        }
                    } else {
                        m_isUsingGuizmo = false;
                    }
                }
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

        // Guizmo Operation Controls
        if (ImGui::CollapsingHeader("Gizmo Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
            const char* operations[] = { "Translate", "Rotate", "Scale" };
            int currentOp = 0;
            
            // Convert current operation to index
            switch (m_guizmoOperation) {
                case ImGuizmo::OPERATION::ROTATE:
                    currentOp = 1;
                    break;
                case ImGuizmo::OPERATION::SCALE:
                    currentOp = 2;
                    break;
                case ImGuizmo::OPERATION::TRANSLATE:
                default:
                    currentOp = 0;
                    break;
            }

            if (ImGui::Combo("Operation", &currentOp, operations, IM_ARRAYSIZE(operations))) {
                // Convert index back to operation
                switch (currentOp) {
                    case 1:
                        m_guizmoOperation = ImGuizmo::OPERATION::ROTATE;
                        break;
                    case 2:
                        m_guizmoOperation = ImGuizmo::OPERATION::SCALE;
                        break;
                    case 0:
                    default:
                        m_guizmoOperation = ImGuizmo::OPERATION::TRANSLATE;
                        break;
                }
            }
        }

        // Geometry list and controls
        if (ImGui::CollapsingHeader("Geometries", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (auto& [name, geom] : m_geometries) {
                ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
                if (m_selectedGeometry == geom.get()) {
                    flags |= ImGuiTreeNodeFlags_Selected;
                }

                bool nodeOpen = ImGui::TreeNodeEx(name.c_str(), flags);
                
                // Handle selection
                if (ImGui::IsItemClicked()) {
                    m_selectedGeometry = (m_selectedGeometry == geom.get()) ? nullptr : geom.get();
                }

                if (nodeOpen) {
                    ImGui::Checkbox("Visible", &geom->visible);

                    if (ImGui::DragFloat3("Position", &geom->position.x, 0.1f)) {
                        // Position was modified through UI
                    }

                    if (ImGui::DragFloat3("Rotation", &geom->rotation.x, 1.0f)) {
                        // Rotation was modified through UI
                    }

                    if (ImGui::DragFloat3("Scale", &geom->scale.x, 0.1f)) {
                        // Scale was modified through UI
                        geom->scale = glm::max(geom->scale, glm::vec3(0.001f)); // Prevent zero or negative scale
                    }

                    if (ImGui::Button("Reset Transform")) {
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