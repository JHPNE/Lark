#pragma once
#include "ProjectBrowserView.h"

#include "Style.h"
#include "Utils/Etc/Logger.h"
#include "Utils/Utils.h"
#include "imgui.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <tinyxml2.h>

namespace detail {
	

    std::string ReadFileContent(const fs::path& path) {
		std::ifstream file(path, std::ios::binary);
        if (!file) return "";
		return std::string(std::istreambuf_iterator<char>(file),
			std::istreambuf_iterator<char>());
    }
}


void ProjectBrowserView::Draw() {
    if (!m_show) return;

	if (Utils::ShowSetEnginePathPopup()) {
		LoadTemplates();
	}

    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Project Browser", &m_show, ImGuiWindowFlags_NoCollapse)) {
        DrawWindowGradientBackground(ImVec4(0.10f,0.10f,0.13f,0.30f), ImVec4(0.10f,0.10f,0.13f,0.80f));
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
        strcpy(pathBuffer, m_projectPath.string().c_str());
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
        CreateNewProject();
    }
}

void ProjectBrowserView::DrawOpenProject() {
    ImGui::BeginChild("OpenProject", ImVec2(0, -30));

    if (m_recentProjects.empty()) {
        ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No recent projects");
    }
    else {
        // Project list on the left
        const float listWidth = 200.0f;
        ImGui::BeginChild("ProjectList", ImVec2(listWidth, 0), true);

        for (size_t i = 0; i < m_recentProjects.size(); i++) {
            const auto& project = m_recentProjects[i];
            if (ImGui::Selectable(project.name.c_str(), m_selectedRecentProject == (int)i)) {
                m_selectedRecentProject = (int)i;
            }

            // Add context menu for each project
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Remove from list")) {
                    m_recentProjects.erase(m_recentProjects.begin() + i);
                    WriteProjectData();
                    if (m_selectedRecentProject >= m_recentProjects.size()) {
                        m_selectedRecentProject = m_recentProjects.empty() ? -1 : 0;
                    }
                }
                ImGui::EndPopup();
            }
        }

        ImGui::EndChild();
        ImGui::SameLine();

        // Project details on the right
        ImGui::BeginChild("ProjectDetails", ImVec2(0, 0), true);
        if (m_selectedRecentProject >= 0 && m_selectedRecentProject < m_recentProjects.size()) {
            const auto& project = m_recentProjects[m_selectedRecentProject];

            ImGui::Text("Name: %s", project.name.c_str());
            ImGui::Text("Path: %s", project.path.string().c_str());
            ImGui::Text("Last Opened: %s", project.date.c_str());

            // Check if project file exists
            if (!fs::exists(project.GetFullPath())) {
                ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Project file not found!");
            }
        }
        ImGui::EndChild();
    }

	ImGui::EndChild(); // End of OpenProject child

    // Open button
    ImGui::BeginDisabled(m_selectedRecentProject < 0);
    if (ImGui::Button("Open Project", ImVec2(-1, 0))) {
        OpenSelectedProject();
    }
    ImGui::EndDisabled();
}

void ProjectBrowserView::LoadRecentProjects() {
	m_appDataPath = Utils::GetApplicationDataPath();
	m_projectDataPath = m_appDataPath / "ProjectData.xml";

	if (!fs::exists(m_appDataPath)) {
		fs::create_directories(m_appDataPath);
	}

    // Sort m_recentProjects
    std::sort(m_recentProjects.begin(), m_recentProjects.end(),
        [](const ProjectData& a, const ProjectData& b) {
			return a.date > b.date;
		});
    
    ReadProjectData();
}

bool ProjectBrowserView::ReadProjectData() {
    Logger::Get().Log(MessageType::Info, "ReadProjectData called");

    m_recentProjects.clear();
    Logger::Get().Log(MessageType::Info, "Cleared existing projects");

    if (!fs::exists(m_projectDataPath)) {
        Logger::Get().Log(MessageType::Warning,
            "Project data file not found: " + m_projectDataPath.string());
        return false;
    }

    try {
        tinyxml2::XMLDocument doc;
        if (doc.LoadFile(m_projectDataPath.string().c_str()) != tinyxml2::XML_SUCCESS) {
            Logger::Get().Log(MessageType::Error,
                "Failed to load project data file: " + m_projectDataPath.string());
            return false;
        }

        auto root = doc.FirstChildElement("ProjectDataList");
        if (!root) {
            Logger::Get().Log(MessageType::Error, "No ProjectDataList element found");
            return false;
        }

        auto projectsElement = root->FirstChildElement("Projects");
        if (!projectsElement) {
            Logger::Get().Log(MessageType::Error, "No Projects element found");
            return false;
        }

        int count = 0;
        for (auto element = projectsElement->FirstChildElement("ProjectData");
             element;
             element = element->NextSiblingElement("ProjectData")) {
            count++;

            ProjectData data;
            auto dateElement = element->FirstChildElement("Date");
            auto nameElement = element->FirstChildElement("ProjectName");
            auto pathElement = element->FirstChildElement("ProjectPath");

            if (!dateElement || !nameElement || !pathElement) {
                Logger::Get().Log(MessageType::Error, "Missing required elements in ProjectData");
                continue;
            }

            data.date = dateElement->GetText() ? dateElement->GetText() : "";
            data.name = nameElement->GetText() ? nameElement->GetText() : "";
            data.path = pathElement->GetText() ? pathElement->GetText() : "";

            if (fs::exists(data.GetFullPath())) {
                m_recentProjects.push_back(data);
                Logger::Get().Log(MessageType::Info, "Added project to list");
            } else {
                Logger::Get().Log(MessageType::Warning,
                    "Project file not found: " + data.GetFullPath().string());
            }
        }

        Logger::Get().Log(MessageType::Info,
            "Found " + std::to_string(count) + " projects in XML, " +
            "Added " + std::to_string(m_recentProjects.size()) + " valid projects");

        // Sort by date
        if (!m_recentProjects.empty()) {
            std::sort(m_recentProjects.begin(), m_recentProjects.end(),
                [](const ProjectData& a, const ProjectData& b) {
                    return a.date > b.date;
                });
        }

        return true;
    }
    catch (const std::exception& e) {
        Logger::Get().Log(MessageType::Error,
            "Error reading project data: " + std::string(e.what()));
        return false;
    }
}

// TODO: seperate update and write we dont wanna rewrite everytime
bool ProjectBrowserView::WriteProjectData() {
    try {
        if (!fs::exists(m_appDataPath)) {
            fs::create_directories(m_appDataPath);
        }

        tinyxml2::XMLDocument doc;
        SerializationContext context(doc);

        auto decl = doc.NewDeclaration();
        doc.LinkEndChild(decl);

        auto root = doc.NewElement("ProjectDataList");
        root->SetAttribute("xmlns", "http://schemas.datacontract.org/2004/07/DrosimEditor.SimProject");
        root->SetAttribute("xmlns:i", "http://www.w3.org/2001/XMLSchema-instance");
        doc.LinkEndChild(root);

        auto projectsElement = doc.NewElement("Projects");
        root->LinkEndChild(projectsElement);

        for (const auto& project : m_recentProjects) {
            if (project.name.empty() || project.path.empty()) continue;
            if (!fs::exists(project.path)) continue;

            auto projectElement = doc.NewElement("ProjectData");
            project.Serialize(projectElement, context);
            projectsElement->LinkEndChild(projectElement);
        }

        return doc.SaveFile(m_projectDataPath.string().c_str()) == tinyxml2::XML_SUCCESS;
    }
    catch (const std::exception& e) {
        Logger::Get().Log(MessageType::Error,
            "Error writing project data: " + std::string(e.what()));
        return false;
    }
}

void ProjectBrowserView::LoadTemplates() {

	std::string enginePathString = Utils::GetEnvVar("LARK_ENGINE");

	if (enginePathString.empty()) {
		Utils::s_showEnginePathPopup = true;
		Logger::Get().Log(MessageType::Error, "Engine path not set");
        return;
	}

    // TODO: Make template path configurable
    fs::path templatePath = Utils::GetEngineResourcePath();
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

void ProjectBrowserView::OpenSelectedProject() {
    if (m_selectedRecentProject >= 0 && m_selectedRecentProject < m_recentProjects.size()) {
        auto& projectData = m_recentProjects[m_selectedRecentProject];

        // Check if project file exists
        if (!fs::exists(projectData.GetFullPath())) {
            Logger::Get().Log(MessageType::Error,
                "Project file not found: " + projectData.GetFullPath().string());
            return;
        }

        if (auto project = Project::Load(projectData.GetFullPath())) {
            // Update last opened time
            auto now = std::chrono::system_clock::now();
            auto timeT = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S");
            projectData.date = ss.str();

            // Move this project to the top of the list
            std::rotate(
                m_recentProjects.begin(),
                m_recentProjects.begin() + m_selectedRecentProject,
                m_recentProjects.begin() + m_selectedRecentProject + 1
            );

            // load project instance
            m_loadedProject = project;

            // Save updated project data
            WriteProjectData();

            Logger::Get().Log(MessageType::Info,
                "Project opened successfully: " + projectData.name);
            m_show = false;
        }
    }
}

bool ProjectBrowserView::CreateNewProject() {
    if (!ValidateProjectPath() || m_selectedTemplate >= m_templates.size()) {
        return false;
    }

    auto tmpl = m_templates[m_selectedTemplate];
    if (auto project = Project::Create(m_newProjectName, m_projectPath, *tmpl)) {
        ProjectData projectData;
        projectData.name = m_newProjectName;
        projectData.path = m_projectPath / m_newProjectName;

        // Get current time
        auto now = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S");
        projectData.date = ss.str();

        m_recentProjects.push_back(projectData);
        m_loadedProject = project;
        WriteProjectData();

        Logger::Get().Log(MessageType::Info, "Project created successfully");
        m_show = false;
        return true;
    }

    return false;
}