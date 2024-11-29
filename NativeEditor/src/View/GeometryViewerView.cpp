#include "GeometryViewerView.h"
#include "../Utils/ImGuizmoManager.h"

#include <iostream>

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

    // Initialize ImGuizmo for this frame
    ImGuizmo::BeginFrame();

    ImGui::PushID("GeometryViewerMain");
    if (ImGui::Begin("Geometry Viewer##Main", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();

        if (viewportSize.x > 0 && viewportSize.y > 0) {
            SetUpViewport();

            // Create view matrix with camera position and rotation
            glm::mat4 view = glm::mat4(1.0f);
            
            // Calculate camera position and target in world space
            glm::vec3 cameraPos = glm::vec3(m_cameraPosition[0], m_cameraPosition[1], m_cameraPosition[2]);
            glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f);
            glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
            
            // Apply camera rotations in the correct order
            glm::mat4 rotation = glm::mat4(1.0f);
            rotation = glm::rotate(rotation, glm::radians(m_cameraRotation[0]), glm::vec3(1.0f, 0.0f, 0.0f)); // Pitch
            rotation = glm::rotate(rotation, glm::radians(m_cameraRotation[1]), glm::vec3(0.0f, 1.0f, 0.0f)); // Yaw
            rotation = glm::rotate(rotation, glm::radians(m_cameraRotation[2]), glm::vec3(0.0f, 0.0f, 1.0f)); // Roll
            
            forward = glm::vec3(rotation * glm::vec4(forward, 0.0f));
            up = glm::vec3(rotation * glm::vec4(up, 0.0f));
            
            // Create the view matrix
            glm::vec3 actualCameraPos = cameraPos - (forward * m_cameraDistance);
            view = glm::lookAt(actualCameraPos, cameraPos, up);

            float aspectRatio = viewportSize.x / viewportSize.y;
            //TODO move those to Settings
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);

            // Save OpenGL state
            GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
            GLint last_framebuffer; glGetIntegerv(GL_FRAMEBUFFER_BINDING, &last_framebuffer);

            // Bind our framebuffer and set viewport
            glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
            glViewport(0, 0, (GLsizei)viewportSize.x, (GLsizei)viewportSize.y);

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

            // Get the screen-space position of the viewport
            ImVec2 windowPos = ImGui::GetWindowPos();
            ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
            ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
            ImVec2 canvasPos = ImVec2(windowPos.x + contentMin.x, windowPos.y + contentMin.y);
            ImVec2 canvasSize = ImVec2(contentMax.x - contentMin.x, contentMax.y - contentMin.y);

            if (m_colorTexture) {
                // Draw the rendered image
                ImGui::GetWindowDrawList()->AddImage(
                    reinterpret_cast<ImTextureID>((void*)(uintptr_t)m_colorTexture),
                    canvasPos,
                    ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y)
                );

                // Handle viewport clicking for selection
                if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGuizmo::IsOver()) {
                    if (!m_geometries.empty()) {
                        auto it = m_geometries.begin();
                        m_selectedGeometry = it->second.get();
                    }
                }

                // Set up ImGuizmo
                ImGuizmo::SetDrawlist();
                ImGuizmo::SetRect(canvasPos.x, canvasPos.y, canvasSize.x, canvasSize.y);
                ImGuizmo::SetOrthographic(false);
                ImGuizmo::Enable(true);

                // Draw Guizmo for selected geometry
                if (m_selectedGeometry) {
                    // Create model matrix for selected geometry
                    glm::mat4 model = glm::mat4(1.0f);

                    glm::quat rotationQuat = glm::quat(glm::radians(m_selectedGeometry->rotation));
                    model = glm::translate(model, m_selectedGeometry->position);
                    model = model * glm::mat4_cast(rotationQuat);
                    model = glm::scale(model, m_selectedGeometry->scale);

                    // Convert matrices for ImGuizmo
                    float modelMatrix[16], viewMatrix[16], projMatrix[16];
                    
                    // Ensure matrices are in column-major order for ImGuizmo
                    glm::mat4 imguizmoView = view;
                    glm::mat4 imguizmoProj = projection;
                    
                    memcpy(modelMatrix, glm::value_ptr(model), sizeof(float) * 16);
                    memcpy(viewMatrix, glm::value_ptr(imguizmoView), sizeof(float) * 16);
                    memcpy(projMatrix, glm::value_ptr(imguizmoProj), sizeof(float) * 16);

                    // Draw the gizmo with snap values
                    float snapValues[3] = { 0.1f, 0.1f, 0.1f }; // Translation, Rotation, Scale
                    
                    // Create a corrected view matrix for ImGuizmo
                    glm::mat4 correctedView = imguizmoView;
                    // Invert Y-axis transformation in the view matrix
                    correctedView[1][1] *= -1;
                    memcpy(viewMatrix, glm::value_ptr(correctedView), sizeof(float) * 16);
                    
                    bool manipulated = ImGuizmo::Manipulate(
                        viewMatrix,
                        projMatrix,
                        (ImGuizmo::OPERATION)m_guizmoOperation,
                        ImGuizmo::MODE::LOCAL,  // Changed to LOCAL mode for more intuitive manipulation
                        modelMatrix,
                        nullptr,
                        snapValues,
                        nullptr,
                        nullptr
                    );

                    // If the matrix was modified, update the geometry transform
                    if (manipulated) {
                        m_isUsingGuizmo = true;
                        glm::mat4 newModel = glm::make_mat4(modelMatrix);


                        // Extract position, rotation, and scale
                        glm::vec3 skew;
                        glm::vec4 perspective;
                        glm::quat rotation;
                        
                        if (glm::decompose(newModel, m_selectedGeometry->scale, rotation, m_selectedGeometry->position, skew, perspective)) {
                            // Convert quaternion to euler angles (in radians)
                            glm::vec3 rotationRadians = glm::eulerAngles(rotation);
                            
                            // Convert rotation to degrees
                            m_selectedGeometry->rotation = glm::degrees(rotationRadians);

                            // Ensure scale doesn't go negative
                            m_selectedGeometry->scale = glm::max(m_selectedGeometry->scale, glm::vec3(0.001f)); // Prevent zero or negative scale
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
            ImGui::DragFloat3("Camera Rotation", m_cameraRotation, 1.0f);
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
    m_cameraRotation[0] = m_cameraRotation[1] = m_cameraRotation[2] = 0.0f;
    m_cameraDistance = 10.0f;
}

void GeometryViewerView::ResetGeometryTransform(ViewportGeometry* geom) {
    if (!geom) return;
    geom->position = glm::vec3(0.0f);
    geom->rotation = glm::vec3(0.0f);
    geom->scale = glm::vec3(1.0f);
}