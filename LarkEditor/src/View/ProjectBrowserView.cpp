#include "ProjectBrowserView.h"
#include "../ViewModels/ProjectBrowserViewModel.h"
#include "../Style/CustomWidgets.h"
#include "../Style/Theme.h"
#include <imgui.h>
#include <imgui_internal.h>
#include <cstring>

using namespace LarkStyle;

ProjectBrowserView::ProjectBrowserView()
    : m_viewModel(std::make_unique<ProjectBrowserViewModel>())
{
}

ProjectBrowserView::~ProjectBrowserView() = default;

void ProjectBrowserView::Draw()
{
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.05f, 0.05f, 0.06f, 1.0f));

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration |
                                   ImGuiWindowFlags_NoMove |
                                   ImGuiWindowFlags_NoResize |
                                   ImGuiWindowFlags_NoSavedSettings |
                                   ImGuiWindowFlags_NoBringToFrontOnFocus;

    bool open = ImGui::Begin("ProjectBrowser", nullptr, window_flags);
    if (open)
    {
        DrawBackground();

        ImVec2 windowSize = ImGui::GetWindowSize();
        float splitPos = windowSize.x * 0.45f;

        DrawLeftPanel(splitPos);
        DrawRightPanel(splitPos);
        DrawStatusBar();
    }
    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(3);

    if (m_viewModel->ShouldCloseWindow())
    {
        m_loadedProject = m_viewModel->LoadedProject.Get();
        m_shouldTransition = true;
    }
}

void ProjectBrowserView::DrawBackground()
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();

    draw_list->AddRectFilledMultiColor(
        pos,
        ImVec2(pos.x + size.x, pos.y + size.y),
        IM_COL32(13, 13, 15, 255),
        IM_COL32(13, 13, 15, 255),
        IM_COL32(20, 20, 25, 255),
        IM_COL32(20, 20, 25, 255)
    );

    float gridSize = 30.0f;
    ImU32 gridColor = IM_COL32(255, 255, 255, 5);

    for (float x = pos.x; x < pos.x + size.x; x += gridSize)
        draw_list->AddLine(ImVec2(x, pos.y), ImVec2(x, pos.y + size.y), gridColor);

    for (float y = pos.y; y < pos.y + size.y; y += gridSize)
        draw_list->AddLine(ImVec2(pos.x, y), ImVec2(pos.x + size.x, y), gridColor);
}

void ProjectBrowserView::DrawLeftPanel(float width)
{
    ImGui::SetCursorPos(ImVec2(50, 100));
    if (ImGui::BeginChild("LeftPanel", ImVec2(width - 100, -150), false))
    {
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "LARK EDITOR");
        ImGui::PopFont();

        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Create New Project");
        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 8));
        ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.11f, 1.0f));

        static char nameBuffer[256] = "";
        if (nameBuffer[0] == '\0')
        {
            strncpy(nameBuffer, m_viewModel->NewProjectName.Get().c_str(), sizeof(nameBuffer) - 1);
            nameBuffer[sizeof(nameBuffer) - 1] = '\0';
        }

        ImGui::Text("Project Name");
        ImGui::SetNextItemWidth(-1);
        if (ImGui::InputText("##ProjectName", nameBuffer, sizeof(nameBuffer)))
            m_viewModel->NewProjectName = std::string(nameBuffer);

        ImGui::Spacing();

        static char pathBuffer[1024] = "";
        if (pathBuffer[0] == '\0')
        {
            std::string pathStr = m_viewModel->NewProjectPath.Get().string();
            strncpy(pathBuffer, pathStr.c_str(), sizeof(pathBuffer) - 1);
            pathBuffer[sizeof(pathBuffer) - 1] = '\0';
        }

        ImGui::Text("Location");
        ImGui::SetNextItemWidth(-100);
        if (ImGui::InputText("##ProjectPath", pathBuffer, sizeof(pathBuffer)))
            m_viewModel->NewProjectPath = fs::path(pathBuffer);

        ImGui::SameLine();
        if (ImGui::Button("Browse", ImVec2(90, 0)))
            m_viewModel->BrowsePathCommand->Execute();

        ImGui::Spacing();

        ImGui::Text("Template");
        auto templates = m_viewModel->Templates.Get();
        if (!templates.empty())
        {
            std::vector<const char*> templateNames;
            for (const auto& tmpl : templates)
                templateNames.push_back(tmpl->GetType().c_str());

            int selected = m_viewModel->SelectedTemplateIndex.Get();
            ImGui::SetNextItemWidth(-1);
            if (ImGui::Combo("##Template", &selected, templateNames.data(), templateNames.size()))
                m_viewModel->SelectedTemplateIndex = selected;
        }

        ImGui::PopStyleColor();
        ImGui::PopStyleVar();

        ImGui::Spacing();
        ImGui::Spacing();

        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.4f, 0.8f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.5f, 0.9f, 1.0f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.35f, 0.75f, 1.0f));

        if (ImGui::Button("Create Project", ImVec2(-1, 40)))
            m_viewModel->CreateProjectCommand->Execute();

        ImGui::PopStyleColor(3);
    }
    ImGui::EndChild();
}

void ProjectBrowserView::DrawRightPanel(float startX)
{
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetWindowPos();

    // Right panel background
    draw_list->AddRectFilled(
        ImVec2(pos.x + startX, pos.y),
        ImVec2(pos.x + windowSize.x, pos.y + windowSize.y),
        IM_COL32(20, 20, 22, 200)
    );

    ImGui::SetCursorPos(ImVec2(startX + 50, 100));
    if (ImGui::BeginChild("RightPanel", ImVec2(windowSize.x - startX - 100, -150), false))
    {
        ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.9f, 1.0f), "Recent Projects");
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        auto recentProjects = m_viewModel->RecentProjects.Get();
        if (recentProjects.empty())
        {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No recent projects");
        }
        else
        {
            for (size_t i = 0; i < recentProjects.size(); ++i)
            {
                const auto& project = recentProjects[i];
                ImGui::PushID(static_cast<int>(i));

                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(15, 10));
                ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.15f, 0.15f, 0.17f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.2f, 0.2f, 0.22f, 1.0f));
                ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.25f, 0.25f, 0.27f, 1.0f));

                bool selected = (m_viewModel->SelectedRecentIndex.Get() == static_cast<int>(i));
                if (ImGui::Selectable("##card", selected, 0, ImVec2(0, 60)))
                    m_viewModel->SelectedRecentIndex = static_cast<int>(i);

                if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0))
                    m_viewModel->OpenProjectCommand->Execute(static_cast<int>(i));

                // --- FIX: draw overlay text via draw list ---
                ImVec2 cardMin = ImGui::GetItemRectMin();
                ImDrawList* dl = ImGui::GetWindowDrawList();

                dl->AddText(ImVec2(cardMin.x + 15, cardMin.y + 10),
                            IM_COL32(230, 230, 230, 255),
                            project.name.c_str());

                dl->AddText(ImVec2(cardMin.x + 15, cardMin.y + 28),
                            IM_COL32(150, 150, 150, 255),
                            project.path.string().c_str());

                std::string lastOpened = "Last opened: " + project.date;
                dl->AddText(ImVec2(cardMin.x + 15, cardMin.y + 44),
                            IM_COL32(130, 130, 130, 255),
                            lastOpened.c_str());
                // --- end fix ---

                if (ImGui::BeginPopupContextItem("ProjectContext"))
                {
                    if (ImGui::MenuItem("Open"))
                        m_viewModel->OpenProjectCommand->Execute(static_cast<int>(i));
                    if (ImGui::MenuItem("Remove from list"))
                        m_viewModel->RemoveRecentCommand->Execute(static_cast<int>(i));
                    ImGui::EndPopup();
                }

                ImGui::PopStyleColor(3);
                ImGui::PopStyleVar();
                ImGui::PopID();
                ImGui::Spacing();
            }
        }
    }
    ImGui::EndChild();
}


void ProjectBrowserView::DrawStatusBar()
{
    ImVec2 windowSize = ImGui::GetWindowSize();
    float barHeight = 30.0f;

    // Position at bottom
    ImGui::SetCursorPosY(windowSize.y - barHeight);

    if (ImGui::BeginChild("StatusBar", ImVec2(windowSize.x, barHeight), false,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        ImDrawList* draw_list = ImGui::GetWindowDrawList();
        ImVec2 pos = ImGui::GetWindowPos();
        ImVec2 size = ImGui::GetWindowSize();

        // Background
        draw_list->AddRectFilled(
            pos,
            ImVec2(pos.x + size.x, pos.y + size.y),
            IM_COL32(25, 25, 28, 255)
        );

        // Text aligned left
        ImGui::SetCursorPos(ImVec2(10, (barHeight - ImGui::GetTextLineHeight()) * 0.5f));
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Status: Ready");
    }
    ImGui::EndChild();
}
