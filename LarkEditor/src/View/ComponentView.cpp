// LarkEditor/src/View/ComponentView.cpp
#include "ComponentView.h"
#include "../ViewModels/ComponentViewModel.h"
#include "Style/CustomWidgets.h"
#include "Style/CustomWindow.h"

#include <imgui.h>

using namespace LarkStyle;

ComponentView::ComponentView() {
    m_viewModel = std::make_unique<ComponentViewModel>();
}

ComponentView::~ComponentView() = default;

void ComponentView::SetActiveProject(std::shared_ptr<Project> activeProject) {
    if (m_viewModel) {
        m_viewModel->SetProject(activeProject);
    }
}

void ComponentView::Draw() {
    if (!m_show || !m_viewModel) return;

    CustomWindow::WindowConfig config;
    config.title = "Component View";
    config.icon = "◈";  // Optional icon, can be empty
    config.p_open = &m_show;
    config.allowDocking = true;
    config.defaultSize = ImVec2(350, 600);
    config.minSize = ImVec2(250, 400);

    if (CustomWindow::Begin("ComponentView", config)) {
        if (m_viewModel->HasSingleSelection.Get()) {
            DrawSingleSelection();
        } else if (m_viewModel->HasMultipleSelection.Get()) {
            DrawMultiSelection();
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No entity selected");
        }
    }
    CustomWindow::End();
}

void ComponentView::DrawSingleSelection() {
    auto entity = m_viewModel->SelectedEntity.Get();
    if (!entity) return;
    
    // Entity header
    CustomWidgets::BeginPanel("EntityInfo", ImVec2(0, 60));
    ImGui::Text("Entity");
    ImGui::SameLine();
    ImGui::TextColored(Colors::AccentInfo, "%s", entity->GetName().c_str());
    ImGui::Text("ID: %u", entity->GetID());
    CustomWidgets::EndPanel();

    // Transform Component
    if (m_viewModel->HasTransform.Get()) {
        DrawTransformComponent();
    }
    
    // Script Component
    if (m_viewModel->HasScript.Get()) {
        DrawScriptComponent();
    }
    
    // Geometry Component
    if (m_viewModel->HasGeometry.Get()) {
        DrawGeometryComponent();
    }
    
    // Add Component Button
    DrawAddComponentButton();
}

void ComponentView::DrawMultiSelection() {
    ImGui::Text("Selected Entities: %zu", m_viewModel->SelectionCount.Get());
    
    for (const auto& entity : m_viewModel->SelectedEntities.Get()) {
        ImGui::BulletText("%s", entity->GetName().c_str());
    }
    
    ImGui::Separator();
    
    // Multi-Transform
    if (m_viewModel->HasTransform.Get()) {
        if (ImGui::CollapsingHeader("Multi-Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
            TransformData avgTransform = m_viewModel->AverageTransform.Get();
            
            // Position
            float pos[3] = {avgTransform.position.x, avgTransform.position.y, avgTransform.position.z};
            if (ImGui::DragFloat3("Position##Multi", pos, 0.1f)) {
                glm::vec3 newPos(pos[0], pos[1], pos[2]);
                m_viewModel->UpdatePositionCommand->Execute(newPos);
            }
            
            // Rotation
            float rot[3] = {avgTransform.rotation.x, avgTransform.rotation.y, avgTransform.rotation.z};
            if (ImGui::DragFloat3("Rotation##Multi", rot, 0.1f)) {
                glm::vec3 newRot(rot[0], rot[1], rot[2]);
                m_viewModel->UpdateRotationCommand->Execute(newRot);
            }
            
            // Scale
            float scale[3] = {avgTransform.scale.x, avgTransform.scale.y, avgTransform.scale.z};
            if (ImGui::DragFloat3("Scale##Multi", scale, 0.1f)) {
                glm::vec3 newScale(scale[0], scale[1], scale[2]);
                m_viewModel->UpdateScaleCommand->Execute(newScale);
            }
        }
    }
    
    // Common Scripts
    if (!m_viewModel->CommonScripts.Get().empty()) {
        if (ImGui::CollapsingHeader("Common Scripts", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (const auto& scriptName : m_viewModel->CommonScripts.Get()) {
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
                ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
                
                ImGui::BeginChild("CommonScriptBox", ImVec2(ImGui::GetContentRegionAvail().x, 30), true);
                ImGui::Text("Script: %s", scriptName.c_str());
                ImGui::EndChild();
                
                ImGui::PopStyleColor();
                ImGui::PopStyleVar();
            }
        }
    }
}

void ComponentView::DrawTransformComponent() {
    if (!CustomWidgets::BeginSection("Transform", true))
        return;
    
    TransformData transform = m_viewModel->CurrentTransform.Get();
    bool changed = false;

    CustomWidgets::BeginPropertyTable();
    
    // Position with X/Y/Z color coding
    float pos[3] = {transform.position.x, transform.position.y, transform.position.z};
    if (CustomWidgets::PropertyFloat3("Position", pos, "%.2f")) {
        transform.position = glm::vec3(pos[0], pos[1], pos[2]);
        changed = true;
    }
    
    // Rotation
    float rot[3] = {transform.rotation.x, transform.rotation.y, transform.rotation.z};
    if (CustomWidgets::PropertyFloat3("Rotation", rot, "%.1f")) {
        transform.rotation = glm::vec3(rot[0], rot[1], rot[2]);
        changed = true;
    }
    
    // Scale with warning color if not uniform
    float scale[3] = {transform.scale.x, transform.scale.y, transform.scale.z};
    bool uniformScale = (scale[0] == scale[1] && scale[1] == scale[2]);

    if (!uniformScale) {
        ImGui::PushStyleColor(ImGuiCol_Text, Colors::AccentWarning);
    }

    if (CustomWidgets::PropertyFloat3("Scale", scale, "%.2f")) {
        transform.scale = glm::vec3(scale[0], scale[1], scale[2]);
        changed = true;
    }

    if (!uniformScale) {
        ImGui::PopStyleColor();
    }

    CustomWidgets::EndPropertyTable();

    // Reset buttons
    ImGui::Spacing();
    if (CustomWidgets::Button("Reset Position", ImVec2(100, 0))) {
        transform.position = glm::vec3(0, 0, 0);
        changed = true;
    }
    ImGui::SameLine();
    if (CustomWidgets::Button("Reset Rotation", ImVec2(100, 0))) {
        transform.rotation = glm::vec3(0, 0, 0);
        changed = true;
    }
    ImGui::SameLine();
    if (CustomWidgets::Button("Reset Scale", ImVec2(100, 0))) {
        transform.scale = glm::vec3(1, 1, 1);
        changed = true;
    }

    if (changed) {
        if (!m_viewModel->IsEditingTransform.Get()) {
            m_viewModel->StartTransformEdit();
        }
        m_viewModel->UpdateTransformCommand->Execute(transform);
    }
    
    // End edit on mouse release
    if (m_viewModel->IsEditingTransform.Get() && ImGui::IsMouseReleased(0)) {
        m_viewModel->EndTransformEdit();
    }

    CustomWidgets::EndSection();
}

void ComponentView::DrawScriptComponent() {
    if (!CustomWidgets::BeginSection("Script", true))
        return;
    
    CustomWidgets::BeginPanel("ScriptInfo", ImVec2(0, 40));
    ImGui::Text("Script: ");
    ImGui::SameLine();
    ImGui::TextColored(Colors::AccentSuccess, "%s", m_viewModel->ScriptName.Get().c_str());
    CustomWidgets::EndPanel();

    ImGui::Spacing();

    if (CustomWidgets::ColoredButton("Remove Script", WidgetColorType::Danger, ImVec2(120, 0))) {
        m_viewModel->RemoveScriptCommand->Execute();
    }

    CustomWidgets::EndSection();
}

void ComponentView::DrawGeometryComponent() {
    if (!CustomWidgets::BeginSection("Geometry", true))
        return;

    CustomWidgets::BeginPropertyTable();

    // Geometry info
    ImGui::Text("Type");
    ImGui::SameLine(Sizing::PropertyLabelWidth);
    ImGui::TextColored(Colors::AccentInfo,
                      m_viewModel->GeometryType.Get() == GeometryType::PrimitiveType ?
                      "Primitive" : "Imported");

    ImGui::Text("Name");
    ImGui::SameLine(Sizing::PropertyLabelWidth);
    ImGui::TextColored(Colors::Text, "%s", m_viewModel->GeometryName.Get().c_str());

    // Visibility with colored indicator
    bool visible = m_viewModel->GeometryVisible.Get();
    CustomWidgets::PropertyBool("Visible", &visible);
    if (visible != m_viewModel->GeometryVisible.Get()) {
        m_viewModel->SetGeometryVisibilityCommand->Execute(visible);
    }

    CustomWidgets::EndPropertyTable();

    ImGui::Spacing();
    CustomWidgets::SeparatorText("Actions");

    if (CustomWidgets::ColoredButton("Randomize Vertices",
                                     WidgetColorType::Warning,
                                     ImVec2(150, 0))) {
        m_viewModel->RandomizeGeometryCommand->Execute();
                                     }

    CustomWidgets::EndSection();
}

void ComponentView::DrawAddComponentButton()
{
    ImGui::Spacing();
    ImGui::Spacing();

    // Center the button
    float buttonWidth = 140.0f;
    float avail = ImGui::GetContentRegionAvail().x;
    float off = (avail - buttonWidth) * 0.5f;
    if (off > 0.0f) ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

    if (CustomWidgets::Button("+ Add Component", ImVec2(buttonWidth, 36))) {
        ImGui::OpenPopup("AddComponentPopup");
    }
}