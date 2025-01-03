#include "GeometryViewerView.h"

void GeometryViewerView::SetActiveProject(std::shared_ptr<Project> activeProject) {
    if (project != activeProject) {
        ClearGeometries();
        project = activeProject;
    }
    m_initialized = true;
}

void GeometryViewerView::LoadExistingGeometry() {
    if (m_loaded) return;

    std::shared_ptr<Scene> activeScene = project->GetActiveScene();
    if (!activeScene) {
        printf("[LoadExistingGeometry] No active scene found.\n");
        return;
    }

    for (auto entity : activeScene->GetEntities()) {
        if (!entity) {
            printf("[LoadExistingGeometry] Found a null entity.\n");
            continue;
        }


        AddGeometry(entity->GetID());
    }
    m_loaded = true;
}

GeometryViewerView::~GeometryViewerView() {
    // Clean up all entities
    for (auto& [name, geom] : m_geometries) {
        if (geom && !Utils::IsInvalidID(geom->entity_id)) {
            RemoveGameEntity(geom->entity_id);
        }
    }
    m_geometries.clear();
}

void GeometryViewerView::ResetGeometryTransform(ViewportGeometry* geom) {
    if (!geom || Utils::IsInvalidID(geom->entity_id)) return;
    ResetEntityTransform(geom->entity_id);
}

// Update the transform getter to include validation
bool GeometryViewerView::GetGeometryTransform(ViewportGeometry* geom, transform_component& transform) {
    if (!geom || Utils::IsInvalidID(geom->entity_id)) return false;
    return GetEntityTransform(geom->entity_id, &transform);
}

//TODO remove unecessary args
void GeometryViewerView::AddGeometry(uint32_t id) {
    auto scene = GetScene(project, id);

    auto buffers = GeometryRenderer::CreateBuffersFromGeometry(scene);
    if (!buffers) {
        printf("[AddGeometry] Failed to create buffers for entity %u.\n", id);
        return;
    }

    m_geometries[id] = std::make_unique<ViewportGeometry>();
    m_geometries[id]->buffers = std::move(buffers);
    m_geometries[id]->entity_id = id;
    m_selectedGeometry = m_geometries[id].get();
}

void GeometryViewerView::UpdateGeometry(uint32_t id) {
    content_tools::SceneData sceneData{};

    GetModifiedMeshData(id, &sceneData);
    if (!sceneData.buffer || sceneData.buffer_size == 0) {
        printf("[UpdateGeometry] Failed to get modified mesh data for entity %u.\n", id);
        return;
    }

    // Loaded new geometry
    auto geometry = new lark::editor::Geometry();
    geometry->FromRawData(sceneData.buffer, sceneData.buffer_size);
    auto scene = geometry->GetScene();

    auto buffers = GeometryRenderer::CreateBuffersFromGeometry(scene);
    if (!buffers) {
        printf("[UpdatedGeometry] Failed to create buffers for entity %u.\n", id);
        return;
    }

    printf("[UpdatedGeometry] Succeeded to create buffers for entity %u.\n", id);
    m_geometries[id] = std::make_unique<ViewportGeometry>();
    m_geometries[id]->buffers = std::move(buffers);
    m_geometries[id]->entity_id = id;
    m_selectedGeometry = m_geometries[id].get();
    m_selectedGeometry = m_geometries[id].get();
}

// replace ViewGeometry name to id
void GeometryViewerView::Draw() {
    if (!m_initialized || !project) {
        printf("[Draw] Skipping draw due to uninitialized state.\n");
        return;
    }

    std::shared_ptr<Scene> activeScene = project->GetActiveScene();
    if (!activeScene) {
        printf("[Draw] No active scene.\n");
        return;
    }

    // If scene has changed, reload geometries
    static uint32_t lastSceneId = 0;
    if (activeScene->GetID() != lastSceneId) {
        ClearGeometries();
        LoadExistingGeometry();
        lastSceneId = activeScene->GetID();
    }

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
            glm::mat4 projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 1000.0f);

            // Save OpenGL state
            GLint last_viewport[4]; glGetIntegerv(GL_VIEWPORT, last_viewport);
            GLint last_framebuffer; glGetIntegerv(GL_FRAMEBUFFER_BINDING, &last_framebuffer);

            // Bind our framebuffer and set viewport
            glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
            glViewport(0, 0, (GLsizei)viewportSize.x, (GLsizei)viewportSize.y);

            // Render all visible geometries
            std::vector<uint32_t> invalidGeometries;
            for (const auto& [id, geom] : m_geometries) {
                std::shared_ptr<Scene> activeScene = project->GetActiveScene();
                auto entity = activeScene->GetEntity(geom->entity_id);
                
                // Mark geometry for removal if entity no longer exists
                if (!entity) {
                    invalidGeometries.push_back(id);
                    continue;
                }

                auto* geometry = entity->GetComponent<Geometry>();
                if (!geometry || !geometry->IsVisible()) continue;

                // Get the transform matrix from the engine
                glm::mat4 model = GetEntityTransformMatrix(geom->entity_id);

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

            // Remove any invalid geometries
            for (uint32_t id : invalidGeometries) {
                RemoveGeometry(id);
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
                for (auto entity : activeScene->GetEntities()) {
                    if (entity && entity->IsSelected()) {
                        // Check if the entity is active
                        if (auto* geometry = entity->GetComponent<Geometry>()) {
                            if (geometry->IsVisible()) {
                                m_selectedGeometry = m_geometries[entity->GetID()].get();
                                // Check if the geometry is visible
                                glm::mat4 model = GetEntityTransformMatrix(m_selectedGeometry->entity_id);

                                // Convert matrices for ImGuizmo
                                float modelMatrix[16], viewMatrix[16], projMatrix[16];
                                memcpy(modelMatrix, glm::value_ptr(model), sizeof(float) * 16);

                                // Create the same view matrix as used for rendering
                                glm::mat4 imguizmoView = view;

                                // Flip the Y-axis for ImGuizmo coordinate system
                                glm::mat4 flipY = glm::mat4(1.0f);
                                flipY[1][1] = -1.0f;

                                // Apply the Y-axis flip to the view matrix
                                imguizmoView = flipY * imguizmoView;

                                memcpy(viewMatrix, glm::value_ptr(imguizmoView), sizeof(float) * 16);
                                memcpy(projMatrix, glm::value_ptr(projection), sizeof(float) * 16);

                                // Draw the gizmo with snap values
                                float snapValues[3] = { 0.1f, 1.0f, 0.1f }; // Translation (0.1), Rotation (1 degree), Scale (0.1)

                                if (ImGuizmo::Manipulate(
                                    viewMatrix,
                                    projMatrix,
                                    (ImGuizmo::OPERATION)m_guizmoOperation,
                                    ImGuizmo::MODE::LOCAL,
                                    modelMatrix,
                                    nullptr,
                                    snapValues
                                )) {
                                    m_isUsingGuizmo = true;
                                    UpdateTransformFromGuizmo(m_selectedGeometry, modelMatrix);
                                } else {
                                    m_isUsingGuizmo = false;
                                }
                            }
                        }
                    }
                };
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
    std::shared_ptr<Scene> activeScene = project->GetActiveScene();
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