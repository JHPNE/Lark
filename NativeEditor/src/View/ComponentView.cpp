#include "ComponentView.h"
#include "../Project/Project.h"  
#include <imgui.h>
#include "../src/Utils/Logger.h"
#include "../src/Utils/Utils.h"
#include "../src/Utils/VectorBox.h"

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
                    float position[3] = {
                        transform->GetPosition().x,
                        transform->GetPosition().y,
                        transform->GetPosition().z
                    };
                    VectorBox positionBox;
                    positionBox.Draw("Position", VectorType::Vector3, position);
					transform->SetPosition({ position[0], position[1], position[2] });

                    ImGui::Spacing();

                    // Rotation
                    float rotation[3] = {
                        transform->GetRotation().x,
                        transform->GetRotation().y,
                        transform->GetRotation().z
                    };
                    VectorBox rotationBox;
                    rotationBox.Draw("Rotation", VectorType::Vector3, rotation);
                    transform->SetRotation({ rotation[0], rotation[1], rotation[2] });

                    ImGui::Spacing();

                    // Scale
                    float scale[3] = {
                        transform->GetScale().x,
                        transform->GetScale().y,
                        transform->GetScale().z
                    };
                    VectorBox scaleBox;
                    scaleBox.Draw("Scale", VectorType::Vector3, scale);
                    transform->SetScale({ scale[0], scale[1], scale[2] });
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