#include "ComponentView.h"
#include "../Project/Project.h"  
#include <imgui.h>
#include "../src/Utils/Utils.h"
#include "../src/Utils/Visual/VectorBox.h"

void ComponentView::Draw() {
    if (!project) {
        return;
    }

    // Always show the window, control visibility through other means if needed
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    window_flags |= ImGuiWindowFlags_NoCollapse;

    // Begin the window - make sure it's visible
    ImGui::Begin("Component View", nullptr, window_flags);

    ImGui::Text("Components");
    ImGui::Separator();

    // Get active scene and selected entity
    std::shared_ptr<Scene> activeScene = project->GetActiveScene();

    if (activeScene) {
        // Find selected entity
        std::shared_ptr<GameEntity> selectedEntity = nullptr;
        for (const auto& entity : activeScene->GetEntities()) {
            if (entity->IsSelected()) {
                selectedEntity = entity;
                break;
            }
        }

        if (selectedEntity) {
            // Entity info header
            ImGui::Text("Selected Entity: %s", selectedEntity->GetName().c_str());
            ImGui::Separator();

            // Get Transform component
            if (auto transform = selectedEntity->GetComponent<Transform>()) {
                if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
                    // Position
                    Vec3 position = transform->GetPosition();
                    float pos[3] = { position.x, position.y, position.z };

                    ImGui::Text("Position");
                    if(VectorBox::Draw("##Position", pos, 3)) {
                        transform->SetPosition({ pos[0], pos[1], pos[2] });
                    }

                    // Rotation
                    Vec3 rotation = transform->GetRotation();
                    float rot[3] = { rotation.x, rotation.y, rotation.z };

                    ImGui::Text("Rotation");
                    if(VectorBox::Draw("##Rotation", rot, 3)) {
                        transform->SetRotation({ rot[0], rot[1], rot[2] });
                    }

                    // Scale
                    Vec3 scale = transform->GetScale();
                    float scl[3] = { scale.x, scale.y, scale.z };

                    ImGui::Text("Scale");
                    if(VectorBox::Draw("##Scale", scl, 3)) {
                        transform->SetScale({ scl[0], scl[1], scl[2] });
                    }
                }
            }

            // Add Component Button
            if (ImGui::Button("Add Component")) {
                ImGui::OpenPopup("AddComponentPopup");
            }

            if (ImGui::BeginPopup("AddComponentPopup")) {
                if (ImGui::MenuItem("Script")) {
                    // Add script component
                    // selectedEntity->AddComponent<Script>();
                }
                // Add other component types here
                ImGui::EndPopup();
            }
        }
        else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No entity selected");
        }
    }
    else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No active scene");
    }

    ImGui::End();
}