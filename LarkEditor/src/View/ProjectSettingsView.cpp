#include "ProjectSettingsView.h"
#include "../ViewModels/ProjectSettingsViewModel.h"
#include "Style/CustomWidgets.h"
#include "Style/CustomWindow.h"
#include "FileDialog.h"
#include <imgui.h>

using namespace LarkStyle;

ProjectSettingsView::ProjectSettingsView()
{
    m_viewModel = std::make_unique<ProjectSettingsViewModel>();
}

ProjectSettingsView::~ProjectSettingsView() = default;

void ProjectSettingsView::SetActiveProject(std::shared_ptr<Project> activeProject)
{
    if (m_viewModel)
    {
        m_viewModel->SetProject(activeProject);
    }
}

void ProjectSettingsView::Draw() {
    if (!m_show || !m_viewModel) return;

    CustomWindow::WindowConfig config;
    config.title = "Project Settings";
    config.icon = "⚙️";
    config.p_open = &m_show;
    config.allowDocking = true;
    config.defaultSize = ImVec2(450, 600);
    config.minSize = ImVec2(350, 400);

    if (CustomWindow::Begin("ProjectSettings", config))
    {
        // Status message
        if (!m_viewModel->StatusMessage.Get().empty())
        {
            ImGui::TextColored(Colors::AccentSuccess, "%s", m_viewModel->StatusMessage.Get().c_str());
            CustomWidgets::Separator();
        }

        if (ImGui::BeginTabBar("ProjectSettingsTabs"))
        {
            // Camera Tab
            if (ImGui::BeginTabItem("Camera"))
            {
                DrawCameraTab();
                ImGui::EndTabItem();
            }

            // Geometry Tab
            if (ImGui::BeginTabItem("Geometry"))
            {
                DrawGeometryTab();
                ImGui::EndTabItem();
            }

            // World Tab
            if (ImGui::BeginTabItem("World"))
            {
                DrawWorldTab();
                ImGui::EndTabItem();
            }

            // Render Tab
            if (ImGui::BeginTabItem("Render"))
            {
                DrawRenderTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        // Save/Load buttons at bottom
        ImGui::Separator();
        ImGui::Spacing();

        if (CustomWidgets::Button("Save Settings", ImVec2(120, 28)))
        {
            m_viewModel->SaveSettingsCommand->Execute();
        }
        ImGui::SameLine();
        if (CustomWidgets::Button("Load Settings", ImVec2(120, 28)))
        {
            m_viewModel->LoadSettingsCommand->Execute();
        }
    }
    CustomWindow::End();
}

void ProjectSettingsView::DrawCameraTab()
{
    CustomWidgets::BeginPropertyGrid("CameraProperties");

    auto cam = m_viewModel->Camera.Get();

    CustomWidgets::SeparatorText("Transform");

    if (CustomWidgets::PropertyFloat3("Position", &cam.position.x))
    {
        m_viewModel->Camera = cam;
    }

    if (CustomWidgets::PropertyFloat3("Rotation", &cam.rotation.x))
    {
        m_viewModel->Camera = cam;
    }

    if (CustomWidgets::PropertyFloat("Distance", &cam.distance, 0.1f, 100.0f))
    {
        m_viewModel->Camera = cam;
    }

    CustomWidgets::SeparatorText("Projection");

    if (CustomWidgets::PropertyFloat("Field of View", &cam.fov, 10.0f, 120.0f))
    {
        m_viewModel->Camera = cam;
    }

    if (CustomWidgets::PropertyFloat("Near Plane", &cam.nearPlane, 0.01f, 10.0f))
    {
        m_viewModel->Camera = cam;
    }

    if (CustomWidgets::PropertyFloat("Far Plane", &cam.farPlane, 10.0f, 10000.0f))
    {
        m_viewModel->Camera = cam;
    }

    CustomWidgets::SeparatorText("Controls");

    if (CustomWidgets::PropertyFloat("Move Speed", &cam.moveSpeed, 0.1f, 50.0f))
    {
        m_viewModel->Camera = cam;
    }

    if (CustomWidgets::PropertyFloat("Rotate Speed", &cam.rotateSpeed, 0.1f, 10.0f))
    {
        m_viewModel->Camera = cam;
    }

    if (CustomWidgets::PropertyFloat("Zoom Speed", &cam.zoomSpeed, 0.1f, 5.0f))
    {
        m_viewModel->Camera = cam;
    }

    CustomWidgets::EndPropertyGrid();

    ImGui::Spacing();

    if (CustomWidgets::AccentButton("Reset Camera", ImVec2(-1, 30)))
    {
        m_viewModel->ResetCameraCommand->Execute();
    }
}

// LarkEditor/src/View/ProjectSettingsView.cpp

void ProjectSettingsView::DrawGeometryTab()
{
    if (!m_viewModel->HasProject.Get())
    {
        ImGui::TextColored(Colors::TextDim, "No project loaded");
        return;
    }

    // Push unique ID for the entire geometry tab to avoid conflicts
    ImGui::PushID("GeometryTabMain");

    // Create Primitive Section
    if (CustomWidgets::BeginSection("Create Primitive", true))
    {
        // Push ID for this section
        ImGui::PushID("PrimitiveSection");

        // Primitive type selection
        const char* types[] = {"Cube", "UV Sphere", "Cylinder"};
        int type = m_viewModel->PrimitiveType.Get();

        ImGui::Text("Type");
        ImGui::SameLine(Sizing::PropertyLabelWidth);
        ImGui::PushItemWidth(Sizing::PropertyControlWidth);

        // Use unique ID for combo
        if (ImGui::Combo("##PrimitiveType", &type, types, IM_ARRAYSIZE(types)))
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

        // Size - with unique IDs
        ImGui::PushID("SizeControls");
        CustomWidgets::BeginPropertyTable();
        glm::vec3 size = m_viewModel->PrimitiveSize.Get();
        if (CustomWidgets::PropertyFloat3("Size", &size.x))
        {
            m_viewModel->PrimitiveSize = size;
        }
        CustomWidgets::EndPropertyTable();
        ImGui::PopID();

        ImGui::Spacing();

        // Segments - with unique IDs for each control
        glm::ivec3 segments = m_viewModel->PrimitiveSegments.Get();
        ImGui::Text("Segments");
        ImGui::Indent();
        ImGui::PushItemWidth(Sizing::PropertyControlWidth);

        ImGui::PushID("SegmentControls");
        switch (type)
        {
            case 0: // Cube
                if (ImGui::DragInt3("##CubeSegments", &segments.x, 1, 1, 10))
                {
                    m_viewModel->PrimitiveSegments = segments;
                }
                break;

            case 1: // Sphere
                ImGui::PushID(0);
                if (ImGui::DragInt("Longitude##SphereSeg", &segments.x, 1, 8, 64))
                {
                    m_viewModel->PrimitiveSegments = segments;
                }
                ImGui::PopID();

                ImGui::PushID(1);
                if (ImGui::DragInt("Latitude##SphereSeg", &segments.y, 1, 4, 32))
                {
                    m_viewModel->PrimitiveSegments = segments;
                }
                ImGui::PopID();
                break;

            case 2: // Cylinder
                ImGui::PushID(0);
                if (ImGui::DragInt("Radial##CylSeg", &segments.x, 1, 8, 64))
                {
                    m_viewModel->PrimitiveSegments = segments;
                }
                ImGui::PopID();

                ImGui::PushID(1);
                if (ImGui::DragInt("Height##CylSeg", &segments.y, 1, 1, 10))
                {
                    m_viewModel->PrimitiveSegments = segments;
                }
                ImGui::PopID();

                ImGui::PushID(2);
                if (ImGui::DragInt("Cap##CylSeg", &segments.z, 1, 1, 5))
                {
                    m_viewModel->PrimitiveSegments = segments;
                }
                ImGui::PopID();
                break;
        }
        ImGui::PopID(); // SegmentControls

        ImGui::PopItemWidth();
        ImGui::Unindent();

        ImGui::Spacing();

        // LOD with unique ID
        int lod = m_viewModel->PrimitiveLOD.Get();
        ImGui::Text("LOD");
        ImGui::SameLine(Sizing::PropertyLabelWidth);
        ImGui::PushItemWidth(Sizing::PropertyControlWidth);
        if (ImGui::SliderInt("##PrimitiveLOD", &lod, 0, 4))
        {
            m_viewModel->PrimitiveLOD = lod;
        }
        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Button with unique ID
        ImGui::PushID("CreateBtn");
        if (CustomWidgets::AccentButton("Create Primitive", ImVec2(-1, 32)))
        {
            m_viewModel->CreatePrimitiveCommand->Execute();
        }
        ImGui::PopID();

        ImGui::PopID(); // PrimitiveSection
        CustomWidgets::EndSection();
    }

    // Import Section with unique ID
    ImGui::PushID("ImportSection");
    CustomWidgets::SeparatorText("Import");

    if (CustomWidgets::Button("Load from File##ImportBtn", ImVec2(-1, 32)))
    {
        m_showFileDialog = true;
    }
    ImGui::PopID(); // ImportSection

    // File dialog handling
    if (m_showFileDialog)
    {
        ImGui::PushID("FileDialogSection");
        static FileDialog fileDialog;
        if (fileDialog.Show(&m_showFileDialog))
        {
            const char* path = fileDialog.GetSelectedPathAsChar();
            if (path && strlen(path) > 0)
            {
                m_viewModel->LoadGeometryCommand->Execute(std::string(path));
            }
        }
        ImGui::PopID(); // FileDialogSection
    }

    ImGui::PopID(); // GeometryTabMain
}

void ProjectSettingsView::DrawWorldTab()
{
    CustomWidgets::BeginPropertyGrid("WorldProperties");

    auto world = m_viewModel->World.Get();

    CustomWidgets::SeparatorText("Gravity");

    if (CustomWidgets::PropertyFloat3("Gravity Vector", &world.gravity.x))
    {
        m_viewModel->World = world;
    }

    CustomWidgets::SeparatorText("Wind");

    const char* windTypes[] = {"No Wind", "Constant", "Sine Gust", "Sine Gust XYZ"};
    int windType = static_cast<int>(world.windType);

    ImGui::Text("Wind Type");
    ImGui::SameLine(Sizing::PropertyLabelWidth);
    ImGui::PushItemWidth(Sizing::PropertyControlWidth);
    if (ImGui::Combo("##WindType", &windType, windTypes, IM_ARRAYSIZE(windTypes)))
    {
        world.windType = static_cast<wind_type>(windType);
        m_viewModel->World = world;
    }
    ImGui::PopItemWidth();

    if (world.windType != wind_type::NoWind)
    {
        if (CustomWidgets::PropertyFloat3("Wind Vector", &world.windVector.x))
        {
            m_viewModel->World = world;
        }

        if (world.windType != wind_type::ConstantWind)
        {
            if (CustomWidgets::PropertyFloat3("Amplitudes", &world.windAmplitudes.x))
            {
                m_viewModel->World = world;
            }

            if (CustomWidgets::PropertyFloat3("Frequencies", &world.windFrequencies.x))
            {
                m_viewModel->World = world;
            }
        }
    }

    CustomWidgets::SeparatorText("Simulation");

    if (CustomWidgets::PropertyFloat("Time Scale", &world.timeScale, 0.0f, 5.0f))
    {
        m_viewModel->World = world;
    }

    int iterations = world.physicsIterations;
    ImGui::Text("Physics Iterations");
    ImGui::SameLine(Sizing::PropertyLabelWidth);
    ImGui::PushItemWidth(Sizing::PropertyControlWidth);
    if (ImGui::DragInt("##PhysIter", &iterations, 1, 1, 100))
    {
        world.physicsIterations = iterations;
        m_viewModel->World = world;
    }
    ImGui::PopItemWidth();

    float fixedStep = world.fixedTimeStep;
    if (CustomWidgets::PropertyFloat("Fixed Time Step", &fixedStep, 0.001f, 0.1f, "%.4f"))
    {
        world.fixedTimeStep = fixedStep;
        m_viewModel->World = world;
    }

    CustomWidgets::EndPropertyGrid();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (CustomWidgets::AccentButton("Apply to Engine", ImVec2(-1, 32)))
    {
        m_viewModel->ApplyWorldSettingsCommand->Execute();
    }
}

void ProjectSettingsView::DrawRenderTab()
{
    CustomWidgets::BeginPropertyGrid("RenderProperties");

    auto render = m_viewModel->Render.Get();

    CustomWidgets::SeparatorText("Display Options");

    if (CustomWidgets::PropertyBool("Enable Wireframe", &render.enableWireframe))
    {
        m_viewModel->Render = render;
    }

    if (CustomWidgets::PropertyBool("Enable Lighting", &render.enableLighting))
    {
        m_viewModel->Render = render;
    }

    if (CustomWidgets::PropertyBool("Enable Shadows", &render.enableShadows))
    {
        m_viewModel->Render = render;
    }

    if (CustomWidgets::PropertyBool("Enable VSync", &render.enableVSync))
    {
        m_viewModel->Render = render;
    }

    CustomWidgets::SeparatorText("Lighting");

    ImGui::Text("Ambient Color");
    ImGui::SameLine(Sizing::PropertyLabelWidth);
    if (ImGui::ColorEdit3("##AmbientColor", &render.ambientColor.x))
    {
        m_viewModel->Render = render;
    }

    if (CustomWidgets::PropertyFloat3("Sun Direction", &render.sunDirection.x))
    {
        // Normalize sun direction
        render.sunDirection = glm::normalize(render.sunDirection);
        m_viewModel->Render = render;
    }

    ImGui::Text("Sun Color");
    ImGui::SameLine(Sizing::PropertyLabelWidth);
    if (ImGui::ColorEdit3("##SunColor", &render.sunColor.x))
    {
        m_viewModel->Render = render;
    }

    if (CustomWidgets::PropertyFloat("Sun Intensity", &render.sunIntensity, 0.0f, 10.0f))
    {
        m_viewModel->Render = render;
    }

    CustomWidgets::EndPropertyGrid();
}
