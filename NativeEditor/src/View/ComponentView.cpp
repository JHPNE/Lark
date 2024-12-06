#include "ComponentView.h"

#include "../Project/Project.h"
#include "Components/Geometry.h"
#include "GeometryViewerView.h"

#include "Components/Script.h"
#include <imgui.h>

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

                    Vec3 position = transform->GetPosition();
                    Vec3 rotation = transform->GetRotation();
                    Vec3 scale = transform->GetScale();

                    transform_component test{};

                    float pos[3];
                    float rot[3];
                    float scl[3];

                    if (GetEntityTransform(selectedEntity->GetID(), &test)) {
                        memcpy(pos, Transform::loadFromEngine(&test), 3 * sizeof(float));
                        memcpy(rot, Transform::loadFromEngine(&test) + 3, 3 * sizeof(float));
                        memcpy(scl, Transform::loadFromEngine(&test) + 6, 3 * sizeof(float));
                    } else {
                        memcpy(pos, Vec3::toFloat(position), 3 * sizeof(float));
                        memcpy(rot, Vec3::toFloat(rotation), 3 * sizeof(float));
                        memcpy(scl, Vec3::toFloat(scale), 3 * sizeof(float));
                    }

                    // Position
                    if (ImGui::DragFloat3("##Position", pos, 0.1f)) {
                        transform->SetPosition({ pos[0], pos[1], pos[2] });
                        transform->packForEngine(&test);
                        SetEntityTransform(selectedEntity->GetID(), test);
                    }

                    // Rotation
                    ImGui::Text("Rotation");
                    if (ImGui::DragFloat3("##Rotation", rot, 0.1f)) {
                        transform->SetRotation({ rot[0], rot[1], rot[2] });
                        transform->packForEngine(&test);
                        SetEntityTransform(selectedEntity->GetID(), test);
                    }

                    // Scale
                    ImGui::Text("Scale");
                    if (ImGui::DragFloat3("##Scale", scl, 0.1f)) {
                        transform->SetScale({ scl[0], scl[1], scl[2] });
                        transform->packForEngine(&test);
                        SetEntityTransform(selectedEntity->GetID(), test);
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

            // Geometry Component(s)
            if (auto* geometry = selectedEntity->GetComponent<Geometry>()) {
                if (ImGui::CollapsingHeader("Geometry", ImGuiTreeNodeFlags_DefaultOpen)) {
                    // Style the script name box
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

                    // Create a box showing the script name
                    ImGui::BeginChild("GeometryBox", ImVec2(ImGui::GetContentRegionAvail().x, 30), true);
                    ImGui::Text("Geometry: %s", geometry->GetGeometryName().c_str());

                    ImGui::EndChild();

                    bool isVisible = geometry->IsVisible();
                    if (ImGui::Checkbox("Visible", &isVisible)) {
                        geometry->SetVisible(isVisible);
                    }

                    ImGui::PopStyleColor();
                    ImGui::PopStyleVar();
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
        else if (selectedEntities.size() > 1) {
            ImGui::Text("Selected Entities: %d", selectedEntities.size());
            std::vector<Vec3> positions;
            std::vector<Vec3> rotations;
            std::vector<Vec3> scales;

            std::vector<std::string> scriptsNames;

            for (auto& entity : selectedEntities) {
                ImGui::Text("%s", entity->GetName().c_str());
                positions.push_back(entity->GetComponent<Transform>()->GetPosition());
                rotations.push_back(entity->GetComponent<Transform>()->GetRotation());
                scales.push_back(entity->GetComponent<Transform>()->GetScale());

                if (entity->GetComponent<Script>()) {
                    scriptsNames.push_back(entity->GetComponent<Script>()->GetScriptName());
                }
            }

            ImGui::Separator();

            Vec3 middlePosition = Vec3::getAverage(positions);
            Vec3 middleRotation = Vec3::getAverage(rotations);
            Vec3 middleScale = Vec3::getAverage(scales);
            float pos[3]{};
            float rot[3]{};
            float scale[3]{};
            if (ImGui::CollapsingHeader("MultiTransform", ImGuiTreeNodeFlags_DefaultOpen)) {
                pos[0] = middlePosition.x; pos[1] = middlePosition.y; pos[2] = middlePosition.z;
                ImGui::DragFloat3("##Position", pos, 0.1f);

                rot[0] = middleRotation.x; rot[1] = middleRotation.y; rot[2] = middleRotation.z;
                ImGui::DragFloat3("##Rotation", rot, 0.1f);

                scale[0] = middleScale.x; scale[1] = middleScale.y; scale[2] = middleScale.z;
                ImGui::DragFloat3("##Scale", scale, 0.1f);
            }

            if (ImGui::CollapsingHeader("MultiScript", ImGuiTreeNodeFlags_DefaultOpen)) {
                std::string scriptCommonName = "";
                for (auto& scriptName : scriptsNames) {
                    bool isCommon = true;
                    for (auto& entity : selectedEntities) {
                        auto* script = entity->GetComponent<Script>();

                        if (!script || script->GetScriptName() != scriptName) {
                            isCommon = false;
                            break;
                        }
                    }
                    if (isCommon) {
                        scriptCommonName = scriptName;
                    }
                }

                if (scriptCommonName != "") {
                    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(5, 5));
                    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.15f, 0.15f, 0.15f, 1.0f));

                    // Create a box showing the script name
                    ImGui::BeginChild("ScriptBox", ImVec2(ImGui::GetContentRegionAvail().x, 30), true);
                    ImGui::Text("Script: %s", scriptCommonName.c_str());
                    ImGui::EndChild();

                    ImGui::PopStyleColor();
                    ImGui::PopStyleVar();

                    // Remove script button
                    if (ImGui::Button("Remove Script", ImVec2(120, 0))) {
                        //activeScene->RemoveComponentFromEntity<Script>(selectedEntity->GetID());
                    }
                }
            }

            for (auto& entity : selectedEntities) {
                if (auto transform = entity->GetComponent<Transform>()) {
                    Vec3 newPosition(pos[0], pos[1], pos[2]);
                    Vec3 newRotation(rot[0], rot[1], rot[2]);
                    Vec3 newScale(scale[0], scale[1], scale[2]);

                    transform_component package{};
                    // Only update position if there's a change
                    if (!Vec3::IsEqual(newPosition, middlePosition)) {
                        Vec3 differencePosition = middlePosition - newPosition;
                        transform->SetPosition(transform->GetPosition() + differencePosition);
                        transform->packForEngine(&package);
                        SetEntityTransform(entity->GetID(), package);
                    }

                    // Only update rotation if there's a change
                    if (!Vec3::IsEqual(newRotation, middleRotation)) {
                        Vec3 differenceRotation = middleRotation - newRotation;
                        transform->SetRotation(transform->GetRotation() + differenceRotation);
                        transform->packForEngine(&package);
                        SetEntityTransform(entity->GetID(), package);
                    }

                    // Only update scale if there's a change
                    if (!Vec3::IsEqual(newScale, middleScale)) {
                        Vec3 differenceScale = middleScale - newScale;
                        transform->SetScale(transform->GetScale() + differenceScale);
                        transform->packForEngine(&package);
                        SetEntityTransform(entity->GetID(), package);
                    }
                }
            }
        } else {
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No entity selected");
        }
    }
    else {
        ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "No active scene");
    }

    ImGui::End();
}
