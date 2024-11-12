#include "SceneView.h"
#include "../Project/Project.h"  
#include <imgui.h>
#include "../src/Utils/Logger.h"
#include "../src/Utils/Utils.h"

void SceneView::Draw() {
    if (!m_show || !project)
        return;

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    window_flags |= ImGuiWindowFlags_NoCollapse;

    if (ImGui::Begin("Scene Manager", &m_show, window_flags)) {
        if (ImGui::Button("+ Add Scene")) {
            static int sceneCounter = 1;
            std::string sceneName = "New Scene";
            project->AddScene(sceneName);
        }
        ImGui::Separator();

        // Get a copy of scenes to iterate over
        auto scenes = project->GetScenes();
        uint32_t sceneToDelete;

        // First pass: render UI and mark scene for deletion
        for (const auto& scene : scenes) {
            ImGui::TextUnformatted(scene->GetName().c_str());
            ImGui::SameLine();
            if (ImGui::Button(("Delete##" + scene->GetName()).c_str())) {
                sceneToDelete = scene->GetID(); // Mark for deletion instead of deleting immediately
            }
        }

        // Second pass: handle deletion after iteration
        if (Utils::IsInvalidID(sceneToDelete)) {
            project->RemoveScene(sceneToDelete);
        }
    }
    ImGui::End();
}