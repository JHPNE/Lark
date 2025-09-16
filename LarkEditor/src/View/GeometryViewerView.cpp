#include "GeometryViewerView.h"
#include "../ViewModels/GeometryViewModel.h"
#include "FileDialog.h"
#include <ImGuizmo.h>

#include "Style/CustomWidgets.h"
#include "Style/CustomWindow.h"

using namespace LarkStyle;

GeometryViewerView::GeometryViewerView() = default;

GeometryViewerView::~GeometryViewerView()
{
    if (m_framebuffer) glDeleteFramebuffers(1, &m_framebuffer);
    if (m_colorTexture) glDeleteTextures(1, &m_colorTexture);
    if (m_depthTexture) glDeleteTextures(1, &m_depthTexture);
}

void GeometryViewerView::SetActiveProject(std::shared_ptr<Project> activeProject)
{
    if (!m_viewModel)
    {
        m_viewModel = std::make_unique<GeometryViewModel>();
    }
    m_viewModel->SetProject(std::move(activeProject));

    m_initialized = true;
}

void GeometryViewerView::Draw()
{
    if (!m_initialized || !m_viewModel)
        return;

    // Initialize ImGuizmo for this frame
    ImGuizmo::BeginFrame();

    DrawViewport();
    DrawControls();
}

void GeometryViewerView::EnsureFramebuffer(float width, float height)
{
    // If framebuffer already exists with correct size, return
    if (m_framebuffer && m_colorTexture && m_depthTexture)
    {
        GLint texWidth, texHeight;
        glBindTexture(GL_TEXTURE_2D, m_colorTexture);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &texWidth);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &texHeight);

        if (texWidth == (GLint)width && texHeight == (GLint)height)
        {
            return;
        }
    }

    // Clean up existing framebuffer if it exists
    if (m_framebuffer)
    {
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
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)width, (GLsizei)height, 0, GL_RGBA,
                 GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_colorTexture, 0);

    // Create depth texture
    glGenTextures(1, &m_depthTexture);
    glBindTexture(GL_TEXTURE_2D, m_depthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, (GLsizei)width, (GLsizei)height, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_depthTexture, 0);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        printf("Framebuffer incomplete! Status: 0x%x\n", status);
        printf("Width: %.0f, Height: %.0f\n", width, height);

        // Clean up
        if (m_framebuffer)
        {
            glDeleteFramebuffers(1, &m_framebuffer);
            glDeleteTextures(1, &m_colorTexture);
            glDeleteTextures(1, &m_depthTexture);
            m_framebuffer = 0;
            m_colorTexture = 0;
            m_depthTexture = 0;
        }
        return;
    }

    // Unbind the framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GeometryViewerView::DrawViewport()
{
    ImGui::PushID("GeometryViewerMain");
    if (ImGui::Begin("Geometry Viewer##Main", nullptr,
                     ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();

        if (viewportSize.x > 0 && viewportSize.y > 0)
        {
            // Check OpenGL error state before proceeding
            GLenum err = glGetError();
            if (err != GL_NO_ERROR)
            {
                printf("OpenGL error before viewport setup: 0x%x\n", err);
            }

            EnsureFramebuffer(viewportSize.x, viewportSize.y);

            // Setup viewport
            GLint last_viewport[4];
            glGetIntegerv(GL_VIEWPORT, last_viewport);
            GLint last_framebuffer;
            glGetIntegerv(GL_FRAMEBUFFER_BINDING, &last_framebuffer);

            glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);
            glViewport(0, 0, (GLsizei)viewportSize.x, (GLsizei)viewportSize.y);

            glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glEnable(GL_DEPTH_TEST);
            glEnable(GL_CULL_FACE);
            glCullFace(GL_BACK);

            // Calculate view and projection matrices
            glm::mat4 view = CalculateViewMatrix();
            glm::mat4 projection = glm::perspective(
                glm::radians(45.0f),
                viewportSize.x / viewportSize.y,
                0.1f, 1000.0f);

            // Render all geometries
            auto& renderManager = m_viewModel->GetRenderManager();
            const auto& model = m_viewModel->GetModel();

            size_t visibleCount = 0;
            for (const auto& [entityId, geomInstance] : model.GetAllGeometries())
            {
                if (geomInstance->visible)
                    visibleCount++;
            }

            static int frameCounter = 0;
            if (frameCounter++ % 60 == 0) // Log every 60 frames
            {
                printf("[DrawViewport] Rendering %zu/%zu visible geometries\n",
                       visibleCount, model.GetAllGeometries().size());
            }

            for (const auto& [entityId, geomInstance] : model.GetAllGeometries())
            {
                if (!geomInstance->visible)
                    continue;

                glm::mat4 transform = m_viewModel->GetEntityTransform(entityId);
                glm::mat4 finalView = view * transform;

                renderManager.RenderGeometry(
                    entityId, finalView, projection,
                    m_viewModel->CameraDistance.Get());
            }

            // Restore OpenGL state
            glBindFramebuffer(GL_FRAMEBUFFER, last_framebuffer);
            glViewport(last_viewport[0], last_viewport[1],
                      (GLsizei)last_viewport[2], (GLsizei)last_viewport[3]);

            err = glGetError();
            if (err != GL_NO_ERROR)
            {
                printf("OpenGL error after rendering: 0x%x\n", err);
            }

            // Draw rendered image
            ImVec2 windowPos = ImGui::GetWindowPos();
            ImVec2 contentMin = ImGui::GetWindowContentRegionMin();
            ImVec2 canvasPos = ImVec2(windowPos.x + contentMin.x, windowPos.y + contentMin.y);
            ImVec2 canvasSize = viewportSize;

            if (m_colorTexture)
            {
                ImGui::GetWindowDrawList()->AddImage(
                    reinterpret_cast<ImTextureID>((void*)(uintptr_t)m_colorTexture),
                    canvasPos,
                    ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y));

                // Handle selection
                if (ImGui::IsWindowHovered() && ImGui::IsMouseClicked(0) && !ImGuizmo::IsOver())
                {
                    // Simple selection - select first geometry
                    // In a real implementation, you'd do ray-casting
                    const auto& geometries = model.GetAllGeometries();
                    if (!geometries.empty())
                    {
                        m_viewModel->SelectEntityCommand->Execute(geometries.begin()->first);
                    }
                }

                // Draw gizmo for selected entity
                DrawGizmo(canvasPos, canvasSize, view, projection);
            }
        }
    }
    ImGui::End();
    ImGui::PopID();
}

void GeometryViewerView::DrawGizmo(const ImVec2& canvasPos, const ImVec2& canvasSize,
                                   const glm::mat4& view, const glm::mat4& projection)
{
    uint32_t selectedId = m_viewModel->SelectedEntityId.Get();
    if (selectedId == static_cast<uint32_t>(-1))
        return;

    // Set up ImGuizmo
    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(canvasPos.x, canvasPos.y, canvasSize.x, canvasSize.y);
    ImGuizmo::SetOrthographic(false);
    ImGuizmo::Enable(true);

    // Get current transform
    glm::mat4 model = m_viewModel->GetEntityTransform(selectedId);

    // Convert matrices for ImGuizmo
    float modelMatrix[16], viewMatrix[16], projMatrix[16];
    memcpy(modelMatrix, glm::value_ptr(model), sizeof(float) * 16);

    // Flip Y-axis for ImGuizmo
    glm::mat4 imguizmoView = glm::mat4(1.0f);
    imguizmoView[1][1] = -1.0f;
    imguizmoView = imguizmoView * view;

    memcpy(viewMatrix, glm::value_ptr(imguizmoView), sizeof(float) * 16);
    memcpy(projMatrix, glm::value_ptr(projection), sizeof(float) * 16);

    // Map operation
    ImGuizmo::OPERATION operation;
    switch (m_viewModel->GizmoOperation.Get())
    {
    case 1: operation = ImGuizmo::OPERATION::ROTATE; break;
    case 2: operation = ImGuizmo::OPERATION::SCALE; break;
    default: operation = ImGuizmo::OPERATION::TRANSLATE; break;
    }

    // Draw and handle gizmo
    float snapValues[3] = {0.1f, 0.1f, 0.1f};

    if (ImGuizmo::Manipulate(viewMatrix, projMatrix, operation,
                            ImGuizmo::MODE::LOCAL, modelMatrix,
                            nullptr, snapValues))
    {
        m_viewModel->IsUsingGizmo = true;
        m_viewModel->UpdateTransformFromGizmo(selectedId, modelMatrix);
    }
    else
    {
        m_viewModel->IsUsingGizmo = false;
    }
}

void GeometryViewerView::DrawControls()
{
    CustomWindow::WindowConfig config;
    config.title = "Geometry Controls";
    config.icon = "⚙️";
    config.p_open = nullptr;
    config.allowDocking = true;
    config.defaultSize = ImVec2(350, 500);
    config.minSize = ImVec2(250, 400);

    ImGui::PushID("GeometryViewerControls");
    if (CustomWindow::Begin("GeometryControls", config))
    {
        // Status
        if (!m_viewModel->StatusMessage.Get().empty())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::AccentSuccess);
            ImGui::Text("%s", m_viewModel->StatusMessage.Get().c_str());
            ImGui::PopStyleColor();
            CustomWidgets::Separator();
        }

        // Camera controls
        if (CustomWidgets::BeginSection("Camera", true))
        {
            CustomWidgets::BeginPropertyTable();

            glm::vec3 pos = m_viewModel->CameraPosition.Get();
            if (CustomWidgets::PropertyFloat3("Position", &pos.x))
            {
                m_viewModel->CameraPosition = pos;
            }

            glm::vec3 rot = m_viewModel->CameraRotation.Get();
            if (CustomWidgets::PropertyFloat3("Rotation", &rot.x))
            {
                m_viewModel->CameraRotation = rot;
            }

            float dist = m_viewModel->CameraDistance.Get();
            if (CustomWidgets::PropertyFloat("Distance", &dist, 0.1f, 100.0f))
            {
                m_viewModel->CameraDistance = dist;
            }

            CustomWidgets::EndPropertyTable();

            if (CustomWidgets::Button("Reset Camera", ImVec2(-1, 28)))
            {
                m_viewModel->ResetCameraCommand->Execute();
            }

            CustomWidgets::EndSection();
        }

        // Gizmo controls
        if (CustomWidgets::BeginSection("Transform", true))
        {
            const char* operations[] = {"Translate", "Rotate", "Scale"};
            int op = m_viewModel->GizmoOperation.Get();

            // Use raw ImGui for combo but without PropertyTable
            ImGui::Text("Operation");
            ImGui::SameLine(Sizing::PropertyLabelWidth);
            ImGui::PushItemWidth(Sizing::PropertyControlWidth);
            if (ImGui::Combo("##Operation", &op, operations, IM_ARRAYSIZE(operations)))
            {
                m_viewModel->GizmoOperation = op;
            }
            ImGui::PopItemWidth();

            if (m_viewModel->HasSelection.Get())
            {
                ImGui::Spacing();
                ImGui::Text("Selected Entity: %u", m_viewModel->SelectedEntityId.Get());
                ImGui::Spacing();

                if (CustomWidgets::ColoredButton("Randomize Vertices",
                                                WidgetColorType::Warning,
                                                ImVec2(-1, 30)))
                {
                    m_viewModel->RandomizeVerticesCommand->Execute();
                }
            }
            else
            {
                ImGui::TextColored(Colors::TextDim, "No selection");
            }

            CustomWidgets::EndSection();
        }

        // Geometry creation
        if (CustomWidgets::BeginSection("Create Geometry", false))
        {
            // Primitive type - Use raw ImGui without PropertyTable
            const char* types[] = {"Cube", "UV Sphere", "Cylinder"};
            int type = m_viewModel->PrimitiveType.Get();

            ImGui::Text("Type");
            ImGui::SameLine(Sizing::PropertyLabelWidth);
            ImGui::PushItemWidth(Sizing::PropertyControlWidth);
            if (ImGui::Combo("##Type", &type, types, IM_ARRAYSIZE(types)))
            {
                m_viewModel->PrimitiveType = type;

                // Update default segments based on type
                switch (type)
                {
                    case 0: // Cube
                        m_viewModel->PrimitiveSegments = glm::ivec3(1, 1, 1);
                        break;
                    case 1: // Sphere
                        m_viewModel->PrimitiveSegments = glm::ivec3(32, 16, 1);
                        break;
                    case 2: // Cylinder
                        m_viewModel->PrimitiveSegments = glm::ivec3(32, 1, 1);
                        break;
                }
            }
            ImGui::PopItemWidth();

            ImGui::Spacing();

            // Size - Use PropertyFloat3
            CustomWidgets::BeginPropertyTable();
            glm::vec3 size = m_viewModel->PrimitiveSize.Get();
            if (CustomWidgets::PropertyFloat3("Size", &size.x))
            {
                m_viewModel->PrimitiveSize = size;
            }
            CustomWidgets::EndPropertyTable();

            ImGui::Spacing();

            // Segments - Use raw ImGui without PropertyTable
            glm::ivec3 segments = m_viewModel->PrimitiveSegments.Get();

            ImGui::Text("Segments");
            ImGui::Indent();
            ImGui::PushItemWidth(Sizing::PropertyControlWidth);

            switch (type)
            {
                case 0: // Cube
                    if (ImGui::DragInt3("##Segments", &segments.x, 1, 1, 10))
                    {
                        m_viewModel->PrimitiveSegments = segments;
                    }
                    break;

                case 1: // Sphere
                    if (ImGui::DragInt("Longitude##Seg", &segments.x, 1, 8, 64))
                    {
                        m_viewModel->PrimitiveSegments = segments;
                    }
                    if (ImGui::DragInt("Latitude##Seg", &segments.y, 1, 4, 32))
                    {
                        m_viewModel->PrimitiveSegments = segments;
                    }
                    break;

                case 2: // Cylinder
                    if (ImGui::DragInt("Radial##Seg", &segments.x, 1, 8, 64))
                    {
                        m_viewModel->PrimitiveSegments = segments;
                    }
                    if (ImGui::DragInt("Height##Seg", &segments.y, 1, 1, 10))
                    {
                        m_viewModel->PrimitiveSegments = segments;
                    }
                    if (ImGui::DragInt("Cap##Seg", &segments.z, 1, 1, 5))
                    {
                        m_viewModel->PrimitiveSegments = segments;
                    }
                    break;
            }

            ImGui::PopItemWidth();
            ImGui::Unindent();

            ImGui::Spacing();

            // LOD
            int lod = m_viewModel->PrimitiveLOD.Get();
            ImGui::Text("LOD");
            ImGui::SameLine(Sizing::PropertyLabelWidth);
            ImGui::PushItemWidth(Sizing::PropertyControlWidth);
            if (ImGui::SliderInt("##LOD", &lod, 0, 4))
            {
                m_viewModel->PrimitiveLOD = lod;
            }
            ImGui::PopItemWidth();

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (CustomWidgets::AccentButton("Create Primitive", ImVec2(-1, 32)))
            {
                m_viewModel->CreatePrimitiveCommand->Execute();
            }

            CustomWidgets::SeparatorText("Import");

            // Load from file
            if (CustomWidgets::Button("Load from File", ImVec2(-1, 32)))
            {
                m_showFileDialog = true;
            }

            CustomWidgets::EndSection();
        }

        // File dialog handling outside of section
        if (m_showFileDialog)
        {
            static FileDialog fileDialog;
            if (fileDialog.Show(&m_showFileDialog))
            {
                const char* path = fileDialog.GetSelectedPathAsChar();
                if (path && strlen(path) > 0)
                {
                    m_viewModel->LoadGeometryCommand->Execute(std::string(path));
                }
            }
        }
    }
    CustomWindow::End();
    ImGui::PopID();
}

glm::mat4 GeometryViewerView::CalculateViewMatrix()
{
    glm::vec3 cameraPos = m_viewModel->CameraPosition.Get();
    glm::vec3 cameraRot = m_viewModel->CameraRotation.Get();
    float distance = m_viewModel->CameraDistance.Get();

    glm::vec3 forward(0.0f, 0.0f, -1.0f);
    glm::vec3 up(0.0f, 1.0f, 0.0f);

    // Apply rotations
    glm::mat4 rotation = glm::mat4(1.0f);
    rotation = glm::rotate(rotation, glm::radians(cameraRot.x), glm::vec3(1.0f, 0.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(cameraRot.y), glm::vec3(0.0f, 1.0f, 0.0f));
    rotation = glm::rotate(rotation, glm::radians(cameraRot.z), glm::vec3(0.0f, 0.0f, 1.0f));

    forward = glm::vec3(rotation * glm::vec4(forward, 0.0f));
    up = glm::vec3(rotation * glm::vec4(up, 0.0f));

    glm::vec3 actualCameraPos = cameraPos - (forward * distance);
    return glm::lookAt(actualCameraPos, cameraPos, up);
}