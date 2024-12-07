#include <filesystem>
#include "imgui.h"
#include "Style.h"

namespace fs = std::filesystem;

class FileDialog {
public:
    FileDialog() : selectedPath(""), currentPath(fs::current_path()) {}

    bool Show(bool* isOpen) {
        if (!*isOpen) return false;

        ImGui::Begin("File Dialog", isOpen, ImGuiWindowFlags_AlwaysAutoResize);
        DrawCurrentPath();
        DrawFileList();
        DrawControls();
        ImGui::End();

        return pathSelected;
    }

    const char* GetSelectedPathAsChar() const {
        return selectedPath.c_str();
    }

private:
    bool pathSelected = false;
    std::string selectedPath;
    fs::path currentPath;

    void DrawCurrentPath() {
        ImGui::Text("Current Path:");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(0.26f, 0.59f, 0.98f, 1.0f), "%s", currentPath.string().c_str());
        ImGui::Separator();
    }

    void DrawFileList() {
        ImGui::BeginChild("FileList", ImVec2(400, 300), true);

        if (currentPath.has_parent_path()) {
            if (ImGui::Selectable("..", false)) {
                currentPath = currentPath.parent_path();
            }
        }

        for (const auto& entry : fs::directory_iterator(currentPath)) {
            const auto& path = entry.path();
            bool isDir = entry.is_directory();
            const std::string& label = isDir ? (path.filename().string() + "/") : path.filename().string();

            if (ImGui::Selectable(label.c_str(), false)) {
                if (isDir) {
                    currentPath = path;
                } else {
                    selectedPath = path.string();
                    pathSelected = true;
                }
            }
        }

        ImGui::EndChild();
    }

    void DrawControls() {
        if (ImGui::Button("Cancel")) {
            pathSelected = false;
            ImGui::CloseCurrentPopup();
        }

        ImGui::SameLine();

        if (ImGui::Button("Select")) {
            if (!selectedPath.empty()) {
                pathSelected = true;
                ImGui::CloseCurrentPopup();
            }
        }
    }
};

// Example usage
void ShowFileDialogExample() {
    static bool showDialog = false;
    static FileDialog fileDialog;

    ApplyModernDarkStyle();

    if (ImGui::Begin("Main Window")) {
        if (ImGui::Button("Open File Dialog")) {
            showDialog = true;
        }
    }
    ImGui::End();

    if (showDialog) {
        if (fileDialog.Show(&showDialog)) {
            const char* selectedPath = fileDialog.GetSelectedPathAsChar();
            // Do something with the selectedPath
        }
    }
}