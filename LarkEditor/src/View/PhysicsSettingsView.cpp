#include "PhysicsSettingsView.h"

#include "Components/Geometry.h"
#include "Geometry/Geometry.h"
#include "Project/Project.h"
#include "Project/Scene.h"
#include "View/GeometryViewerView.h"
#include "View/Style.h"
#include <imgui.h>

void PhysicsSettingsView::SetActiveProject(std::shared_ptr<Project> activeProject)
{
    m_project = activeProject;
}

void PhysicsSettingsView::Draw()
{
    if (!m_project)
        return;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    window_flags |= ImGuiWindowFlags_NoCollapse;

    ImGui::Text("Physics Settings", nullptr, window_flags);
    DrawWindowGradientBackground(ImVec4(0.1f, 0.1f, 0.13f, 0.30f),
                                 ImVec4(0.1f, 0.1f, 0.13f, 0.80f));

    ImGui::Text("Physics Settings");
    ImGui::Separator();

    std::shared_ptr<Scene> activeScene = m_project->GetActiveScene();

    if (!activeScene)
    {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No active scene");
        ImGui::End();
        return;
    }

    ImGui::Text("Mesh");
    ImGui::Spacing();

    ImGui::DragFloat("##Arm Length", &m_arm_length, 0.25f);
    ImGui::DragFloat("##Mass", &m_mass, 1.0f);

    if (ImGui::Button("Create Drone", ImVec2(-1, 30)))
    {
        CreateDrone();
    }

    ImGui::End();
}

void PhysicsSettingsView::CreateDrone() { return; }
