#include "SceneView.h"
#include "../Project/Project.h"  
#include <imgui.h>
#include "../src/Utils/Logger.h"
#include "../src/Utils/Utils.h"

namespace detail {
    uint32_t INVALIDID = -1;
}

void SceneView::Draw() {
    if (!m_show || !project)
        return;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    window_flags |= ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("Scene Manager", &m_show, window_flags)) {
        // Add Scene Button at top
        if (ImGui::Button("+ Add Scene")) {
            project->AddScene("New Scene");
        }
        ImGui::Separator();

        // Get scenes and track deletion
        auto scenes = project->GetScenes();
        uint32_t sceneToDelete = detail::INVALIDID;

        // Draw each scene and its entities
        for (const auto& scene : scenes) {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow
                | ImGuiTreeNodeFlags_OpenOnDoubleClick;

            // If this is active scene, add Selected flag
            if (project->GetActiveScene() == scene) {
                flags |= ImGuiTreeNodeFlags_Selected;
            }

            // Scene Tree Node
            bool isOpen = ImGui::TreeNodeEx(
                (scene->GetName() + "##" + std::to_string(scene->GetID())).c_str(),
                flags
            );

            // Scene Context Menu
            if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                ImGui::OpenPopup(("SceneContext##" + std::to_string(scene->GetID())).c_str());
            }

            if (ImGui::BeginPopup(("SceneContext##" + std::to_string(scene->GetID())).c_str())) {
                if (ImGui::MenuItem("Set Active")) {
                    project->SetActiveScene(scene->GetID());
                }
                if (ImGui::MenuItem("Delete")) {
                    sceneToDelete = scene->GetID();
                }
                ImGui::EndPopup();
            }

            // Handle scene selection
            if (ImGui::IsItemClicked()) {
                project->SetActiveScene(scene->GetID());
            }

            // Scene Controls
            if (isOpen) {
                // Add Entity Button
                if (ImGui::Button(("+ Add Entity##" + std::to_string(scene->GetID())).c_str())) {
                    scene->CreateEntity("Empty Entity");
                }

                // List all entities in the scene
                uint32_t entityToDelete = detail::INVALIDID;
                for (const auto& entity : scene->GetEntities()) {
                    ImGuiTreeNodeFlags entityFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

                    // Show enabled/disabled state
                    if (!entity->IsEnabled()) {
                        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
                    }

                    ImGui::TreeNodeEx(
                        (entity->GetName() + "##" + std::to_string(entity->GetID())).c_str(),
                        entityFlags
                    );

                    // Entity Context Menu
                    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
                        ImGui::OpenPopup(("EntityContext##" + std::to_string(entity->GetID())).c_str());
                    }

					if (ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
						entity->SetSelected(!entity->IsSelected());
					}

                    if (ImGui::BeginPopup(("EntityContext##" + std::to_string(entity->GetID())).c_str())) {
                        if (ImGui::MenuItem(entity->IsEnabled() ? "Disable" : "Enable")) {
                            entity->SetEnabled(!entity->IsEnabled());
                        }
                        if (ImGui::MenuItem("Delete")) {
                            entityToDelete = entity->GetID();
                        }
                        ImGui::EndPopup();
                    }

                    if (!entity->IsEnabled()) {
                        ImGui::PopStyleColor();
                    }
                }

                // Handle entity deletion after iteration
                if (entityToDelete != detail::INVALIDID) {
                    scene->RemoveEntity(entityToDelete);
                }

                ImGui::TreePop();
            }
        }

        // Handle scene deletion after iteration
        if (sceneToDelete != detail::INVALIDID) {
            project->RemoveScene(sceneToDelete);
        }
    }
    ImGui::End();
}