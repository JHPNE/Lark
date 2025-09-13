#include "SceneView.h"
#include "../ViewModels/SceneViewModel.h"
#include "Style.h"
#include <imgui.h>

SceneView::SceneView()
{
    m_viewModel = std::make_unique<SceneViewModel>();
}

SceneView::~SceneView() = default;

void SceneView::SetActiveProject(std::shared_ptr<Project> activeProject){
    if (m_viewModel) {
        m_viewModel->SetProject(activeProject);
    }
}

void SceneView::Draw()
{
    if (!m_show || !m_viewModel)
        return;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    window_flags |= ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("Scene Manager", &m_show, window_flags)) {
        DrawWindowGradientBackground(ImVec4(0.10f, 0.10f, 0.13f, 0.30f),
                                     ImVec4(0.10f, 0.10f, 0.13f, 0.80f));
        // Add Scene Button at top
        if (ImGui::Button("+ Add Scene"))
        {
            m_viewModel->AddSceneCommand->Execute();
        }
        ImGui::Separator();

        const auto& hierarchy = m_viewModel->SceneHierarchy.Get();
        for (const auto& sceneNode : hierarchy)
        {
            DrawSceneNode(sceneNode);
        }
    }


    ImGui::End();
}

void SceneView::DrawSceneNode(const SceneNodeData& node)
{
    if (node.isScene)
    {
        // Scene node
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen |
                                  ImGuiTreeNodeFlags_OpenOnArrow;

        if (node.isActive)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.3f, 1.0f));
        }

        bool nodeOpen = ImGui::TreeNodeEx(
            (node.name + "##" + std::to_string(node.id)).c_str(),
            flags);

        if (node.isActive)
        {
            ImGui::PopStyleColor();
        }

        // Context menu
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            ImGui::OpenPopup(("SceneContext##" + std::to_string(node.id)).c_str());
        }
        DrawSceneContextMenu(node.id);

        // Single click to set active
        if (ImGui::IsItemClicked() && !node.isActive)
        {
            m_viewModel->SetActiveSceneCommand->Execute(node.id);
        }

        if (nodeOpen)
        {
            // Add Entity button
            ImGui::Indent();
            if (node.isActive && ImGui::Button(("+ Add Entity##" + std::to_string(node.id)).c_str()))
            {
                m_viewModel->AddEntityCommand->Execute();
            }

            // Draw entities
            for (const auto& entityNode : node.children)
            {
                DrawSceneNode(entityNode);
            }
            ImGui::Unindent();
            ImGui::TreePop();
        }
    }
    else
    {
        // Entity node
        if (!node.isEnabled)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
        }

        std::string label = node.name + "##" + std::to_string(node.id);
        if (ImGui::Selectable(label.c_str(), node.isSelected))
        {
            m_viewModel->SelectEntityCommand->Execute(node.id);
        }

        // Context menu
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        {
            ImGui::OpenPopup(("EntityContext##" + std::to_string(node.id)).c_str());
        }
        DrawEntityContextMenu(node.id);

        if (!node.isEnabled)
        {
            ImGui::PopStyleColor();
        }
    }
}

void SceneView::DrawEntityContextMenu(uint32_t entityId)
{
    if (ImGui::BeginPopup(("EntityContext##" + std::to_string(entityId)).c_str()))
    {
        if (ImGui::MenuItem("Toggle Enabled"))
        {
            m_viewModel->ToggleEntityEnabledCommand->Execute(entityId);
        }
        if (ImGui::MenuItem("Delete"))
        {
            m_viewModel->RemoveEntityCommand->Execute(entityId);
        }
        ImGui::EndPopup();
    }
}

void SceneView::DrawSceneContextMenu(uint32_t sceneId)
{
    if (ImGui::BeginPopup(("SceneContext##" + std::to_string(sceneId)).c_str()))
    {
        if (ImGui::MenuItem("Set Active"))
        {
            m_viewModel->SetActiveSceneCommand->Execute(sceneId);
        }
        if (ImGui::MenuItem("Delete"))
        {
            m_viewModel->RemoveSceneCommand->Execute(sceneId);
        }
        ImGui::EndPopup();
    }
}