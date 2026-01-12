#include "SceneView.h"
#include "../ViewModels/SceneViewModel.h"
#include "Style/CustomWidgets.h"
#include "Style/CustomWindow.h"
#include <imgui.h>
#include <imgui_internal.h>

using namespace LarkStyle;

SceneView::SceneView()
{
    m_viewModel = std::make_unique<SceneViewModel>();
}

SceneView::~SceneView() = default;

void SceneView::SetActiveProject(std::shared_ptr<Project> activeProject)
{
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
    config.icon = "◈";
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
    ImGui::PushID(static_cast<int>(node.id));

    if (node.isScene)
    {
        // Scene node
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen |
                                  ImGuiTreeNodeFlags_OpenOnArrow |
                                  ImGuiTreeNodeFlags_SpanAvailWidth |
                                  ImGuiTreeNodeFlags_FramePadding;

        if (node.isActive)
        {
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::AccentWarning);
        }

        bool nodeOpen = ImGui::TreeNodeEx("##scene", flags, "%s", node.name.c_str());

        if (node.isActive)
        {
            ImGui::PopStyleColor();
        }

        if (ImGui::BeginPopupContextItem("SceneContextMenu"))
        {
            if (ImGui::MenuItem("Set Active")) {
                m_viewModel->SetActiveSceneCommand->Execute(node.id);
            }
            if (ImGui::MenuItem("Delete")) {
                m_viewModel->RemoveSceneCommand->Execute(node.id);
            }
            ImGui::EndPopup();
        }

        // Single click to set active
        if (ImGui::IsItemClicked(ImGuiMouseButton_Left) && !node.isActive)
        {
            m_viewModel->SetActiveSceneCommand->Execute(node.id);
        }

        if (nodeOpen)
        {
            if (node.isActive)
            {
                if (CustomWidgets::Button("+ Add Entity", ImVec2(120, 24)))
                {
                    m_viewModel->AddEntityCommand->Execute();
                }
            }

            // Draw child entities
            for (const auto& entityNode : node.children)
            {
                DrawSceneNode(entityNode);
            }

            ImGui::TreePop();
        }
    }
    else
    {
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf |
                                   ImGuiTreeNodeFlags_NoTreePushOnOpen |
                                   ImGuiTreeNodeFlags_SpanAvailWidth;

        if (node.isSelected) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }

        int styleColorCount = 0;

        if (!node.isEnabled) {
            ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextDisabled);
            styleColorCount++;
        }

        if (node.isSelected) {
            ImGui::PushStyleColor(ImGuiCol_Header, Colors::AccentActive);
            ImGui::PushStyleColor(ImGuiCol_HeaderHovered, Colors::AccentHover);
            ImGui::PushStyleColor(ImGuiCol_HeaderActive, Colors::Accent);
            styleColorCount += 3;
        }

        ImGui::TreeNodeEx("##entity", flags, "%s", node.name.c_str());

        if (styleColorCount > 0) {
            ImGui::PopStyleColor(styleColorCount);
        }

        if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            m_viewModel->SelectEntityCommand->Execute(node.id);
        }
        if (ImGui::BeginPopupContextItem("EntityContextMenu"))
        {
            if (ImGui::MenuItem("Toggle Enabled")) {
                m_viewModel->ToggleEntityEnabledCommand->Execute(node.id);
            }
            if (ImGui::MenuItem("Delete")) {
                m_viewModel->RemoveEntityCommand->Execute(node.id);
            }
            ImGui::EndPopup();
        }
    }

    ImGui::PopID();
}