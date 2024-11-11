#include "SceneView.h"
#include "../Project/Project.h"  
#include <imgui.h>
#include "../src/Utils/Logger.h"

void SceneView::Draw() {
    if (!m_show || !project)
        return;
    if (ImGui::Begin("Scene Manager", &m_show)) {
        if (ImGui::Button("+ Add Scene")) {
            static int sceneCounter = 1;
            std::string sceneName = "Scene " + std::to_string(sceneCounter++);
            project->AddScene(sceneName);
        }
        ImGui::Separator();

        // Get a copy of scenes to iterate over
        auto scenes = project->GetScenes();
        std::string sceneToDelete;

        // First pass: render UI and mark scene for deletion
        for (const auto& scene : scenes) {
            ImGui::TextUnformatted(scene->GetName().c_str());
            ImGui::SameLine();
            if (ImGui::Button(("Delete##" + scene->GetName()).c_str())) {
                sceneToDelete = scene->GetName(); // Mark for deletion instead of deleting immediately
            }
        }

        // Second pass: handle deletion after iteration
        if (!sceneToDelete.empty()) {
            project->RemoveScene(sceneToDelete);
        }
    }
    ImGui::End();
}