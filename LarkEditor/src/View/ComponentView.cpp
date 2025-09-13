// LarkEditor/src/View/ComponentView.cpp
#include "ComponentView.h"
#include "../ViewModels/ComponentViewModel.h"
#include "Style.h"
#include <imgui.h>

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

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse;
    
    if (ImGui::Begin("Component View", &m_show, window_flags)) {
        DrawWindowGradientBackground(ImVec4(0.10f, 0.10f, 0.13f, 0.30f),
                                     ImVec4(0.10f, 0.10f, 0.13f, 0.80f));
        
        ImGui::Text("Components");
        ImGui::Separator();
        
        if (m_viewModel->HasSingleSelection.Get()) {
            DrawSingleSelection();
        } else if (m_viewModel->HasMultipleSelection.Get()) {
            DrawMultiSelection();
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No entity selected");
        }
    }
    ImGui::End();
}

void ComponentView::DrawSingleSelection() {
    auto entity = m_viewModel->SelectedEntity.Get();
    if (!entity) return;
    
    // Entity header
    ImGui::Text("Selected Entity: %s", entity->GetName().c_str());
    ImGui::Separator();
    
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
    if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        return;
    
    TransformData transform = m_viewModel->CurrentTransform.Get();
    bool changed = false;
    
    // Position
    ImGui::Text("Position");
    float pos[3] = {transform.position.x, transform.position.y, transform.position.z};
    if (ImGui::DragFloat3("##Position", pos, 0.1f)) {
        transform.position = glm::vec3(pos[0], pos[1], pos[2]);
        changed = true;
    }
    
    // Rotation
    ImGui::Text("Rotation");
    float rot[3] = {transform.rotation.x, transform.rotation.y, transform.rotation.z};
    if (ImGui::DragFloat3("##Rotation", rot, 0.1f)) {
        transform.rotation = glm::vec3(rot[0], rot[1], rot[2]);
        changed = true;
    }
    
    // Scale
    ImGui::Text("Scale");
    float scale[3] = {transform.scale.x, transform.scale.y, transform.scale.z};
    if (ImGui::DragFloat3("##Scale", scale, 0.1f)) {
        transform.scale = glm::vec3(scale[0], scale[1], scale[2]);
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
}

void ComponentView::DrawScriptComponent() {
    if (!ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen))
        return;
    
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    
    ImGui::BeginChild("ScriptBox", ImVec2(ImGui::GetContentRegionAvail().x, 30), true);
    ImGui::Text("Script: %s", m_viewModel->ScriptName.Get().c_str());
    ImGui::EndChild();
    
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    
    if (ImGui::Button("Remove Script", ImVec2(120, 0))) {
        m_viewModel->RemoveScriptCommand->Execute();
    }
}

void ComponentView::DrawGeometryComponent() {
    if (!ImGui::CollapsingHeader("Geometry", ImGuiTreeNodeFlags_DefaultOpen))
        return;
    
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));
    
    ImGui::BeginChild("GeometryBox", ImVec2(ImGui::GetContentRegionAvail().x, 30), true);
    ImGui::Text("Geometry: %s", m_viewModel->GeometryName.Get().c_str());
    ImGui::EndChild();
    
    ImGui::PopStyleColor();
    ImGui::PopStyleVar();
    
    bool visible = m_viewModel->GeometryVisible.Get();
    if (ImGui::Checkbox("Visible", &visible)) {
        m_viewModel->SetGeometryVisibilityCommand->Execute(visible);
    }
    
    if (ImGui::Button("Randomize Vertices", ImVec2(150, 0))) {
        m_viewModel->RandomizeGeometryCommand->Execute();
    }
}

void ComponentView::DrawAddComponentButton() {
    if (ImGui::Button("Add Component", ImVec2(120, 0))) {
        ImGui::OpenPopup("AddComponentPopup");
    }
    
    if (ImGui::BeginPopup("AddComponentPopup")) {
        // Scripts
        if (!m_viewModel->HasScript.Get() && !m_viewModel->AvailableScripts.Get().empty()) {
            if (ImGui::BeginMenu("Script")) {
                for (const auto& scriptName : m_viewModel->AvailableScripts.Get()) {
                    if (ImGui::MenuItem(scriptName.c_str())) {
                        m_viewModel->AddScriptCommand->Execute(scriptName);
                    }
                }
                ImGui::EndMenu();
            }
        }
        
        // Add other component types here as needed
        
        ImGui::EndPopup();
    }
}