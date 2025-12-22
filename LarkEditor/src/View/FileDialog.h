#include "Style.h"
#include "imgui.h"
#include <filesystem>
#include <algorithm>
#include <string>

namespace fs = std::filesystem;

class FileDialog
{
  public:
    FileDialog() : selectedPath(""), currentPath(fs::current_path()), searchQuery("") {}

    bool Show(bool *isOpen)
    {
        if (!*isOpen)
            return false;

        ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
        ImGui::Begin("Select OBJ File", isOpen, ImGuiWindowFlags_NoCollapse);

        DrawSearchBar();
        ImGui::Spacing();
        DrawCurrentPath();
        ImGui::Spacing();
        DrawFileList();
        ImGui::Spacing();
        bool shouldClose = DrawControls();

        ImGui::End();

        if (shouldClose)
        {
            *isOpen = false;
        }

        return pathSelected;
    }

    const char *GetSelectedPathAsChar() const { return selectedPath.c_str(); }

  private:
    bool pathSelected = false;
    std::string selectedPath;
    fs::path currentPath;
    char searchQuery[256] = "";

    void DrawSearchBar()
    {
        ImGui::PushItemWidth(-1);
        if (ImGui::InputTextWithHint("##search", "Search files and folders...", searchQuery,
                                     IM_ARRAYSIZE(searchQuery)))
        {
        }
        ImGui::PopItemWidth();
    }

    void DrawCurrentPath()
    {
        ImGui::Text("Location:");
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.26f, 0.59f, 0.98f, 1.0f));
        ImGui::TextWrapped("%s", currentPath.string().c_str());
        ImGui::PopStyleColor();
        ImGui::Separator();
    }

    void DrawFileList()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 5.0f);
        ImGui::BeginChild("FileList", ImVec2(0, -40), true, ImGuiWindowFlags_HorizontalScrollbar);

        std::string searchStr = ToLower(searchQuery);
        bool hasSearch = !searchStr.empty();

        if (currentPath.has_parent_path())
        {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));
            if (ImGui::Selectable("📁 ..", false))
            {
                currentPath = currentPath.parent_path();
            }
            ImGui::PopStyleColor();
            ImGui::Separator();
        }

        std::vector<fs::directory_entry> directories;
        std::vector<fs::directory_entry> objFiles;

        try
        {
            for (const auto &entry : fs::directory_iterator(currentPath))
            {
                const auto &path = entry.path();
                std::string filename = path.filename().string();
                std::string filenameLower = ToLower(filename);

                if (hasSearch && filenameLower.find(searchStr) == std::string::npos)
                    continue;

                if (entry.is_directory())
                {
                    directories.push_back(entry);
                }
                else if (path.extension() == ".obj")
                {
                    objFiles.push_back(entry);
                }
            }
        }
        catch (const std::exception &e)
        {
            ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Error reading directory");
        }

        auto sorter = [](const fs::directory_entry &a, const fs::directory_entry &b) {
            return ToLower(a.path().filename().string()) < ToLower(b.path().filename().string());
        };
        std::sort(directories.begin(), directories.end(), sorter);
        std::sort(objFiles.begin(), objFiles.end(), sorter);

        for (const auto &entry : directories)
        {
            const std::string &label = "📁 " + entry.path().filename().string();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.4f, 1.0f));

            if (ImGui::Selectable(label.c_str(), false))
            {
                currentPath = entry.path();
            }

            ImGui::PopStyleColor();
        }

        if (!directories.empty() && !objFiles.empty())
        {
            ImGui::Separator();
        }

        for (const auto &entry : objFiles)
        {
            const auto &path = entry.path();
            const std::string &label = "📄 " + path.filename().string();
            bool isSelected = (selectedPath == path.string());

            if (ImGui::Selectable(label.c_str(), isSelected))
            {
                selectedPath = path.string();
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
            {
                selectedPath = path.string();
                pathSelected = true;
            }
        }

        if (directories.empty() && objFiles.empty())
        {
            ImGui::TextDisabled(hasSearch ? "No matching files or folders" : "No .obj files in this directory");
        }

        ImGui::EndChild();
        ImGui::PopStyleVar();
    }

    bool DrawControls()
    {
        ImGui::Separator();

        if (!selectedPath.empty())
        {
            fs::path selPath(selectedPath);
            ImGui::Text("Selected:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "%s", selPath.filename().string().c_str());
        }

        ImGui::Spacing();

        float buttonWidth = 120.0f;
        float spacing = ImGui::GetStyle().ItemSpacing.x;
        float totalWidth = (buttonWidth * 2) + spacing;
        float offset = ImGui::GetContentRegionAvail().x - totalWidth;

        ImGui::SetCursorPosX(ImGui::GetCursorPosX() + offset);

        bool shouldClose = false;

        if (ImGui::Button("Cancel", ImVec2(buttonWidth, 0)))
        {
            pathSelected = false;
            shouldClose = true;
        }

        ImGui::SameLine();

        ImGui::BeginDisabled(selectedPath.empty());
        if (ImGui::Button("Select", ImVec2(buttonWidth, 0)))
        {
            pathSelected = true;
            shouldClose = true;
        }
        ImGui::EndDisabled();

        return shouldClose;
    }

    static std::string ToLower(const std::string &str)
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(),
                      [](unsigned char c) { return std::tolower(c); });
        return result;
    }
};