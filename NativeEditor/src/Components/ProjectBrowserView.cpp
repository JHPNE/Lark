#include "ProjectBrowserView.h"
#include "Utils/Logger.h"
#include "imgui.h"
#include <filesystem>

void ProjectBrowserView::Draw() {
    if (!m_show) return;

    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Project Browser", &m_show, ImGuiWindowFlags_NoCollapse)) {
        // Tabs for New/Open project
        if (ImGui::BeginTabBar("ProjectTabs")) {
            if (ImGui::BeginTabItem("New Project")) {
                m_isNewProject = true;
                DrawNewProject();
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Open Project")) {
                m_isNewProject = false;
                DrawOpenProject();
                ImGui::EndTabItem();
            }
            ImGui::EndTabBar();
        }
    }
    ImGui::End();
}

void ProjectBrowserView::DrawNewProject() {
    ImGui::BeginChild("NewProject", ImVec2(0, -30)); // Leave space for Create button

    // Project name input - use char buffer for ImGui
    static char nameBuffer[256] = "NewProject";
    ImGui::Text("Project Name");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##ProjectName", nameBuffer, sizeof(nameBuffer))) {
        m_newProjectName = nameBuffer;
    }

    // Project path input - use char buffer for ImGui
    static char pathBuffer[1024] = "";
    if (m_projectPath.string().length() < sizeof(pathBuffer)) {
        strcpy_s(pathBuffer, m_projectPath.string().c_str());
    }
    ImGui::Text("Project Path");
    ImGui::SetNextItemWidth(-1);
    if (ImGui::InputText("##ProjectPath", pathBuffer, sizeof(pathBuffer))) {
        m_projectPath = pathBuffer;
        ValidateProjectPath();
    }

    ImGui::Separator();

    // Templates
    const float templateListWidth = 200.0f;
    ImGui::BeginChild("TemplateList", ImVec2(templateListWidth, 0), true);
    for (size_t i = 0; i < m_templates.size(); i++) {
        if (ImGui::Selectable(m_templates[i]->GetType().c_str(), m_selectedTemplate == i)) {
            m_selectedTemplate = i;
        }
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Template preview
    ImGui::BeginChild("TemplatePreview", ImVec2(0, 0), true);
    if (m_selectedTemplate >= 0 && m_selectedTemplate < m_templates.size()) {
        auto tmpl = m_templates[m_selectedTemplate];
        ImGui::Text("Type: %s", tmpl->GetType().c_str());
        ImGui::Text("Folders:");
        for (const auto& folder : tmpl->GetFolders()) {
            ImGui::BulletText("%s", folder.c_str());
        }
        // TODO: Add screenshot preview when we implement texture loading
    }
    ImGui::EndChild();

    ImGui::EndChild(); // End of NewProject child

    // Create button at the bottom
    if (ImGui::Button("Create Project", ImVec2(-1, 0))) {
        if (ValidateProjectPath() && m_selectedTemplate < m_templates.size()) {
            auto tmpl = m_templates[m_selectedTemplate];
            if (auto project = Project::Create(m_newProjectName, m_projectPath, *tmpl)) {
                Logger::Get().Log(MessageType::Info, "Project created successfully");
                m_show = false;
                // TODO: Set as active project when we implement project management
            }
        }
    }
}

void ProjectBrowserView::DrawOpenProject() {
    // TODO: Implement open project functionality
    ImGui::Text("Open Project functionality coming soon...");
}

void ProjectBrowserView::LoadTemplates() {
    // TODO: Make template path configurable
    fs::path templatePath = fs::current_path() / "ProjectTemplates";
    m_templates = ProjectTemplate::LoadTemplates(templatePath);

    if (m_templates.empty()) {
        Logger::Get().Log(MessageType::Warning,
            "No project templates found in: " + templatePath.string());
    }
}

bool ProjectBrowserView::ValidateProjectPath() {
    try {
        if (m_newProjectName.empty()) {
            Logger::Get().Log(MessageType::Error, "Project name cannot be empty");
            return false;
        }

        if (m_projectPath.empty()) {
            Logger::Get().Log(MessageType::Error, "Project path cannot be empty");
            return false;
        }

        fs::path fullPath = m_projectPath / m_newProjectName;

        if (fs::exists(fullPath)) {
            Logger::Get().Log(MessageType::Error, "Project directory already exists");
            return false;
        }

        return true;
    }
    catch (const std::exception& e) {
        Logger::Get().Log(MessageType::Error,
            "Invalid project path: " + std::string(e.what()));
        return false;
    }
}