#include "ComponentView.h"
#include "../Project/Project.h"  
#include <imgui.h>
#include "Components/Script.h"

void ComponentView::Draw() {
    if (!project) return;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    window_flags |= ImGuiWindowFlags_NoCollapse;

    ImGui::Begin("Component View", nullptr, window_flags);

    ImGui::Text("Components");
    ImGui::Separator();

    std::shared_ptr<Scene> activeScene = project->GetActiveScene();

    if (activeScene) {
        // Find selected entity
        std::vector<std::shared_ptr<GameEntity>> selectedEntities;

        for (const auto& entity : activeScene->GetEntities()) {
            if (entity->IsSelected()) {
                selectedEntities.push_back(entity);
            }
        }

        if (selectedEntities.size() == 1) {
            const std::shared_ptr<GameEntity>& selectedEntity = selectedEntities[0];
            // Entity info header
            ImGui::Text("Selected Entity: %s", selectedEntity->GetName().c_str());
            ImGui::Separator();

            // Transform Component
            if (auto transform = selectedEntity->GetComponent<Transform>()) {
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    // Position
                    Vec3 position = transform->GetPosition();
                    float pos[3] = { position.x, position.y, position.z };
                    if (ImGui::DragFloat3("##Position", pos, 0.1f)) {
                        transform->SetPosition({ pos[0], pos[1], pos[2] });
                    }

                    // Rotation
                    Vec3 rotation = transform->GetRotation();
                    float rot[3] = { rotation.x, rotation.y, rotation.z };
                    ImGui::Text("Rotation");
                    if (ImGui::DragFloat3("##Rotation", rot, 0.1f)) {
                        transform->SetRotation({ rot[0], rot[1], rot[2] });
                    }

                    // Scale
                    Vec3 scale = transform->GetScale();
                    float scl[3] = { scale.x, scale.y, scale.z };
                    ImGui::Text("Scale");
                    if (ImGui::DragFloat3("##Scale", scl, 0.1f)) {
                        transform->SetScale({ scl[0], scl[1], scl[2] });
                    }
                }
            }

            // Script Component(s)
            if (auto script = selectedEntity->GetComponent<Script>()) {
                if (ImGui::CollapsingHeader("Script", ImGuiTreeNodeFlags_DefaultOpen)) {
                    // Style the script name box
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

                    // Create a box showing the script name
                    ImGui::BeginChild("ScriptBox", ImVec2(ImGui::GetContentRegionAvail().x, 30), true);
                    ImGui::Text("Script: %s", script->GetScriptName().c_str());
                    ImGui::EndChild();

                    ImGui::PopStyleColor();
                    ImGui::PopStyleVar();

                    // Remove script button
                    if (ImGui::Button("Remove Script", ImVec2(120, 0))) {
                        //selectedEntity->RemoveComponent<Script>();
                        activeScene->RemoveComponentFromEntity<Script>(selectedEntity->GetID());
                    }
                }
            }

            // Add Component Button (always at bottom)
            if (ImGui::Button("Add Component", ImVec2(120, 0))) {
                ImGui::OpenPopup("AddComponentPopup");
            }

            if (ImGui::BeginPopup("AddComponentPopup")) {
                if (!selectedEntity->GetComponent<Script>()) {  // Only show scripts if none exists
                    size_t script_count = 0;
                    const char** script_names = GetScriptNames(&script_count);

                    if (script_count > 0) {
                        if (ImGui::BeginMenu("Script")) {
                            for (size_t i = 0; i < script_count; i++) {
                                if (ImGui::MenuItem(script_names[i])) {
                                    ScriptInitializer scriptInit;
                                    scriptInit.scriptName = script_names[i];
                                    activeScene->AddComponentToEntity<Script>(selectedEntity->GetID(), &scriptInit);
                                }
                            }
                            ImGui::EndMenu();
                        }
                    }
                }
                ImGui::EndPopup();
            }
        }
        else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No entity selected");
        }

        // Multiselection
        if (selectedEntities.size() > 1) {
            ImGui::Text("Selected Entities: %d", selectedEntities.size());
            for (auto& entity : selectedEntities) {
                ImGui::Text("%s", entity->GetName().c_str());
            }
            ImGui::Separator();

            // MultiTransform Component
        }
    }
    else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No active scene");
    }

    ImGui::End();
}
