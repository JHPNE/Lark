#pragma once
#include "ProjectBrowserView.h"
#include "Utils/Logger.h"
#include "Utils/Utils.h"
#include "imgui.h"
#include <filesystem>
#include <cstdlib>
#include <iostream>
#include <fstream>
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
        if (ValidateProjectPath() && m_selectedTemplate < m_templates.size()) {
            auto tmpl = m_templates[m_selectedTemplate];
            if (auto project = Project::Create(m_newProjectName, m_projectPath, *tmpl)) {
				ProjectData projectData;
				projectData.name = m_newProjectName;
				projectData.path = m_projectPath / m_newProjectName;

                // load project instance
				m_loadedProject = project;

				// Get current time
				auto now = std::chrono::system_clock::now();
				auto timeT = std::chrono::system_clock::to_time_t(now);
                std::stringstream ss;
				ss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S");
				projectData.date = ss.str();

				m_recentProjects.push_back(projectData);
                WriteProjectData();


                Logger::Get().Log(MessageType::Info, "Project created successfully");
                m_show = false;
            }
        }
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

void ProjectBrowserView::OpenSelectedProject() {
    if (m_selectedRecentProject >= 0 && m_selectedRecentProject < m_recentProjects.size()) {
        auto& projectData = m_recentProjects[m_selectedRecentProject];

        // Check if project file exists
        if (!fs::exists(projectData.GetFullPath())) {
            Logger::Get().Log(MessageType::Error, "Project file not found: " + projectData.GetFullPath().string());
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

            Logger::Get().Log(MessageType::Info, "Project opened successfully: " + projectData.name);
            m_show = false;
        }
    }
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
	m_recentProjects.clear();

    if (!fs::exists(m_projectDataPath)) {
        return false;
    }

    try {
        std::string content = detail::ReadFileContent(m_projectDataPath);
        if (content.empty()) {
            Logger::Get().Log(MessageType::Error,
                "Failed to read project data file: " + m_projectDataPath.string());
            return false;
        }

        if (ProjectData::ParseProjectXml(content, m_recentProjects)) {
            //Filter out non-existing projects
            m_recentProjects.erase(
                std::remove_if(m_recentProjects.begin(), m_recentProjects.end(),
                    [](const ProjectData& data) {
                        return !fs::exists(data.GetFullPath());
                    }),
                m_recentProjects.end());

            // Sort by date
            std::sort(m_recentProjects.begin(), m_recentProjects.end(),
                [](const ProjectData& a, const ProjectData& b) {
                    return a.date > b.date;
                });

            return true;
        }
    }
	catch (const std::exception& e) {
		Logger::Get().Log(MessageType::Error,
			"Error reading project data: " + std::string(e.what()));
	}

    return false;
}

// TODO: seperate update and write we dont wanna rewrite everytime
bool ProjectBrowserView::WriteProjectData() {
    try {
        // First ensure the directory exists
        if (!fs::exists(m_appDataPath)) {
            fs::create_directories(m_appDataPath);
        }

		/*
		tinyxml2::XMLDocument doc;
		if (doc.LoadFile(m_projectDataPath.string().c_str()) == tinyxml2::XML_SUCCESS) {
			tinyxml2::XMLElement* root = doc.FirstChildElement("ProjectDataList");
			if (root) {
				root->DeleteChildren();
			}
		}
		*/

        std::stringstream xml;
        xml << "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
        xml << "<ProjectDataList xmlns=\"http://schemas.datacontract.org/2004/07/DrosimEditor.SimProject\" "
            << "xmlns:i=\"http://www.w3.org/2001/XMLSchema-instance\">\n";
        xml << "<Projects>\n";

        for (const auto& project : m_recentProjects) {
            if (project.name.empty() || project.path.empty()) continue;


            if (!fs::exists(project.path.string())) continue;

            xml << "  <ProjectData>\n";
            xml << "    <Date>" << project.date << "</Date>\n";
            xml << "    <ProjectName>" << project.name << "</ProjectName>\n";
            xml << "    <ProjectPath>" << project.path.string() << "</ProjectPath>\n";
            xml << "  </ProjectData>\n";
        }

        xml << "</Projects>\n";
        xml << "</ProjectDataList>";

        // Debug log to see if we get here
        Logger::Get().Log(MessageType::Info,
            "Attempting to write project data to: " + m_projectDataPath.string());


        std::ofstream file(m_projectDataPath, std::ios::binary);
        if (!file || !(file << xml.str())) {
            Logger::Get().Log(MessageType::Error,
                "Failed to write project data: " + m_projectDataPath.string());
            return false;
        }

        return true;
    }
    catch (const std::exception& e) {
        Logger::Get().Log(MessageType::Error,
            "Error writing project data: " + std::string(e.what()));
        return false;
    }
}

void ProjectBrowserView::LoadTemplates() {

	std::string enginePathString = Utils::GetEnvironmentVariable("DRONESIM_ENGINE");

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