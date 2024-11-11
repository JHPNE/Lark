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
            Logger::Get().Log(MessageType::Info, "Added new scene: " + sceneName);
        }

        ImGui::Separator();

        const auto& scenes = project->GetScenes();
        for (const auto& scene : scenes) {
            ImGui::TextUnformatted(scene->GetName().c_str());
            ImGui::SameLine();
            if (ImGui::Button(("Delete##" + scene->GetName()).c_str())) {
                project->RemoveScene(scene->GetName());
                Logger::Get().Log(MessageType::Info, "Deleted scene: " + scene->GetName());
            }
        }
    }
    ImGui::End();
}
