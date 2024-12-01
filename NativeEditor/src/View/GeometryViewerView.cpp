#include "GeometryViewerView.h"
#include "../Utils/ImGuizmoManager.h"
#include "Utils/Utils.h"
#include <iostream>

void GeometryViewerView::AddGeometry(const std::string& name, drosim::editor::Geometry* geometry) {
    if (!geometry) return;

    auto buffers = GeometryRenderer::CreateBuffersFromGeometry(geometry);
    if (buffers) {
        // Create entity descriptor with default transform
        game_entity_descriptor descriptor{};
        descriptor.transform = {}; // Default transform (identity)
        descriptor.script = {};    // No script for now

        // Create entity in engine
        uint32_t entity_id = CreateGameEntity(&descriptor);

        if (!Utils::IsInvalidID(entity_id)) {
            m_geometries[name] = std::make_unique<ViewportGeometry>();
            m_geometries[name]->name = name;
            m_geometries[name]->buffers = std::move(buffers);
            m_geometries[name]->entity_id = entity_id;
            m_geometries[name]->visible = true;
            m_initialized = true;

            // Automatically select the newly added geometry
            m_selectedGeometry = m_geometries[name].get();
        }
    }
}

void GeometryViewerView::RemoveGeometry(const std::string& name) {
    auto it = m_geometries.find(name);
    if (it != m_geometries.end()) {
        RemoveGameEntity(it->second->entity_id);
        m_geometries.erase(it);
    }
}

void GeometryViewerView::Draw() {
    if (!m_initialized) return;

    ImGuizmo::BeginFrame();

    ImGui::PushID("GeometryViewerMain");
    if (ImGui::Begin("Geometry Viewer##Main", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();

        if (viewportSize.x > 0 && viewportSize.y > 0) {
            SetUpViewport();

            // Calculate view and projection matrices
            glm::mat4 view = CalculateViewMatrix();
            float aspectRatio = viewportSize.x / viewportSize.y;
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);

            // Render scene
            RenderScene(view, projection);

            // Display the rendered texture
            if (m_colorTexture) {
                ImVec2 windowPos = ImGui::GetWindowPos();
                ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
                ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
                ImVec2 canvasPos = ImVec2(windowPos.x + contentMin.x, windowPos.y + contentMin.y);
                ImVec2 canvasSize = ImVec2(contentMax.x - contentMin.x, contentMax.y - contentMin.y);

                // Draw the rendered image
                ImGui::GetWindowDrawList()->AddImage(
                    reinterpret_cast<ImTextureID>((void*)(uintptr_t)m_colorTexture),
                    canvasPos,
                    ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                    ImVec2(0, 1),  // UV0
                    ImVec2(1, 0)   // UV1: Flip Y-coordinate to correct OpenGL texture orientation
                );
            }

            // Handle ImGuizmo
            if (m_selectedGeometry) {
                HandleGuizmo(view, projection);
            }
        }
    }
    ImGui::End();
    ImGui::PopID();

    DrawControls();
}

glm::mat4 GeometryViewerView::CalculateViewMatrix() {
    glm::mat4 view = glm::mat4(1.0f);

    glm::vec3 cameraPos = glm::vec3(m_cameraPosition[0], m_cameraPosition[1], m_cameraPosition[2]);
    glm::vec3 forward = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    glm::mat4 rotation = glm::mat4(1.0f);
    rotation = glm::rotate(rotation, glm::radians(m_cameraRotation[0]), glm::vec3(1.0f, 0.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(m_cameraRotation[1]), glm::vec3(0.0f, 1.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(m_cameraRotation[2]), glm::vec3(0.0f, 0.0f, 1.0f));

    forward = glm::vec3(rotation * glm::vec4(forward, 0.0f));
    up = glm::vec3(rotation * glm::vec4(up, 0.0f));

    glm::vec3 actualCameraPos = cameraPos - (forward * m_cameraDistance);
    return glm::lookAt(actualCameraPos, cameraPos, up);
}

void GeometryViewerView::HandleGuizmo(const glm::mat4& view, const glm::mat4& projection) {
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
    ImVec2 contentMax = ImGui::GetWindowContentRegionMax();
    ImVec2 canvasPos(windowPos.x + contentMin.x, windowPos.y + contentMin.y);
    ImVec2 canvasSize(contentMax.x - contentMin.x, contentMax.y - contentMin.y);

    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(canvasPos.x, canvasPos.y, canvasSize.x, canvasSize.y);
    ImGuizmo::SetOrthographic(false);

    // Get current transform from engine
    transform_component current_transform;
    GetEntityTransform(m_selectedGeometry->entity_id, &current_transform);

    // Convert to matrix for ImGuizmo
    glm::mat4 model = GetEntityTransformMatrix(m_selectedGeometry->entity_id);
    float modelMatrix[16], viewMatrix[16], projMatrix[16];

    memcpy(modelMatrix, glm::value_ptr(model), sizeof(float) * 16);
    memcpy(viewMatrix, glm::value_ptr(view), sizeof(float) * 16);
    memcpy(projMatrix, glm::value_ptr(projection), sizeof(float) * 16);

    float snapValues[3] = { 0.1f, 1.0f, 0.1f };

    if (ImGuizmo::Manipulate(
        viewMatrix,
        projMatrix,
        m_guizmoOperation,
        ImGuizmo::MODE::LOCAL,
        modelMatrix,
        nullptr,
        snapValues
    )) {
        // Convert matrix back to transform component
        glm::mat4 newModel = glm::make_mat4(modelMatrix);

        transform_component new_transform{};
        glm::vec3 skew;
        glm::vec4 perspective;
        glm::vec3 translation, scale;
        glm::quat rotation;

        glm::decompose(newModel, scale, rotation, translation, skew, perspective);

        memcpy(new_transform.position, &translation, sizeof(float) * 3);
        glm::vec3 euler = glm::degrees(glm::eulerAngles(rotation));
        memcpy(new_transform.rotation, &euler, sizeof(float) * 3);
        memcpy(new_transform.scale, &scale, sizeof(float) * 3);

        // Update transform in engine
        SetEntityTransform(m_selectedGeometry->entity_id, new_transform);
    }
}

void GeometryViewerView::DrawControls() {
        ImGui::PushID("GeometryViewerControls");
        if (ImGui::Begin("Geometry Controls##ViewerControls")) {
            // Camera controls
            if (ImGui::CollapsingHeader("Camera Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::DragFloat3("Camera Position", m_cameraPosition, 0.1f);
                ImGui::DragFloat3("Camera Rotation", m_cameraRotation, 1.0f);
                ImGui::DragFloat("Camera Distance", &m_cameraDistance, 0.1f, 0.1f, 100.0f);

                if (ImGui::Button("Reset Camera")) {
                    ResetCamera();
                }
            }

            // Guizmo controls
            if (ImGui::CollapsingHeader("Gizmo Controls", ImGuiTreeNodeFlags_DefaultOpen)) {
                const char* operations[] = { "Translate", "Rotate", "Scale" };
                int currentOp = (int)m_guizmoOperation - 1;
                if (ImGui::Combo("Operation", &currentOp, operations, IM_ARRAYSIZE(operations))) {
                    m_guizmoOperation = (ImGuizmo::OPERATION)(currentOp + 1);
                }
            }

            // Geometry list
            if (ImGui::CollapsingHeader("Geometries", ImGuiTreeNodeFlags_DefaultOpen)) {
                for (auto& [name, geom] : m_geometries) {
                    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow;
                    if (m_selectedGeometry == geom.get()) {
                        flags |= ImGuiTreeNodeFlags_Selected;
                    }

                    bool nodeOpen = ImGui::TreeNodeEx(name.c_str(), flags);

                    if (ImGui::IsItemClicked()) {
                        m_selectedGeometry = geom.get();
                    }

                    if (nodeOpen) {
                        ImGui::Checkbox("Visible", &geom->visible);

                        transform_component transform;
                        GetEntityTransform(geom->entity_id, &transform);

                        if (ImGui::DragFloat3("Position", transform.position, 0.1f) ||
                            ImGui::DragFloat3("Rotation", transform.rotation, 1.0f) ||
                            ImGui::DragFloat3("Scale", transform.scale, 0.1f)) {
                            SetEntityTransform(geom->entity_id, transform);
                        }

                        if (ImGui::Button("Reset Transform")) {
                            ResetEntityTransform(geom->entity_id);
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
    if (viewportSize.x <= 0 || viewportSize.y <= 0) {
        std::cout << "Invalid viewport size!\n";
        return;
    }

    EnsureFramebuffer(viewportSize.x, viewportSize.y);

    glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

    // Check framebuffer status
    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cout << "Framebuffer is not complete! Status: " << status << "\n";
        return;
    }

    glViewport(0, 0, (GLsizei)viewportSize.x, (GLsizei)viewportSize.y);

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Check for GL errors
    GLenum err = glGetError();
    if (err != GL_NO_ERROR) {
        std::cout << "OpenGL error in SetUpViewport: " << err << "\n";
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}

void GeometryViewerView::RenderScene(const glm::mat4& view, const glm::mat4& projection) {
    std::cout << "RenderScene called with " << m_geometries.size() << " geometries\n";

    if (!m_framebuffer || !m_colorTexture) {
        std::cout << "Invalid framebuffer or color texture!\n";
        return;
    }

    GLint last_viewport[4];
    glGetIntegerv(GL_VIEWPORT, last_viewport);
    GLint last_framebuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &last_framebuffer);

    for (const auto& [name, geom] : m_geometries) {
        if (!geom->visible) continue;
        if (!geom->buffers) {
            std::cout << "Null buffers for geometry: " << name << "\n";
            continue;
        }

        std::cout << "Rendering geometry: " << name << "\n";

        // Get transform matrix from engine
        glm::mat4 model = GetEntityTransformMatrix(geom->entity_id);
        glm::mat4 finalView = view * model;

        GeometryRenderer::RenderGeometryAtLOD(
            geom->buffers.get(),
            finalView,
            projection,
            m_cameraDistance
        );

        // Check for GL errors after rendering
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            std::cout << "OpenGL error after rendering " << name << ": " << err << "\n";
        }
    }
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