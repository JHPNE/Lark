#include "SceneView.h"
#include "../ViewModels/SceneViewModel.h"
#include "Style/CustomWidgets.h"
#include "Style/CustomWindow.h"
#include <imgui.h>

using namespace LarkStyle;

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

    CustomWindow::WindowConfig config;
    config.title = "Scene Manager";
    config.icon = "◈";  // Optional icon, can be empty
    config.p_open = &m_show;
    config.allowDocking = true;
    config.defaultSize = ImVec2(350, 600);
    config.minSize = ImVec2(250, 400);

    if (CustomWindow::Begin("SceneManager", config)) {
        if (CustomWidgets::AccentButton("+ Add Scene", ImVec2(100, 0)))
        {
            m_viewModel->AddSceneCommand->Execute();
        }

        CustomWidgets::Separator();

        const auto& hierarchy = m_viewModel->SceneHierarchy.Get();
        for (const auto& sceneNode : hierarchy)
        {
            DrawSceneNode(sceneNode);
        }
    }

    CustomWindow::End();
}

void SceneView::DrawSceneNode(const SceneNodeData& node)
{
    if (node.isScene)
    {
        // Scene node
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen |
                                  ImGuiTreeNodeFlags_OpenOnArrow |
                                  ImGuiTreeNodeFlags_SpanAvailWidth;

        if (node.isActive) {
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::AccentWarning);
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
            if (node.isActive)
            {
                if (CustomWidgets::Button(("+ Add Entity##" + std::to_string(node.id)).c_str(),
                                         ImVec2(120, 24)))
                {
                    m_viewModel->AddEntityCommand->Execute();
                }
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
        int pushedColors = 0;

        // Entity node with custom styling
        if (!node.isEnabled) {
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDisabled);
            pushedColors++;
        }

        if (node.isSelected) {
            ImGui::PushStyleColor(ImGuiCol_Header, Colors::AccentActive);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Colors::AccentHover);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, Colors::Accent);
            pushedColors += 3;
        }

        std::string label = node.name + "##" + std::to_string(node.id);
        if (ImGui::Selectable(label.c_str(), node.isSelected)) {
            m_viewModel->SelectEntityCommand->Execute(node.id);
        }

        if (pushedColors > 0) {
            ImGui::PopStyleColor(pushedColors);
        }

        // Context menu
        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup(("EntityContext##" + std::to_string(node.id)).c_str());
        }
        DrawEntityContextMenu(node.id);
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