#pragma once
#include "ObservableProperty.h"
#include "../Commands/ICommand.h"
#include "../Project/Project.h"
#include "../Project/ProjectData.h"
#include "../Project/ProjectTemplate.h"
#include "../Utils/Utils.h"
#include "../Utils/System/Serialization.h"
#include <filesystem>
#include <memory>
#include <vector>
#include <tinyxml2.h>
#include <chrono>
#include <iomanip>
#include <sstream>

class ProjectBrowserViewModel
{
public:
    // Observable properties
    ObservableProperty<bool> IsCreatingNew{true};
    ObservableProperty<std::string> NewProjectName{"NewProject"};
    ObservableProperty<fs::path> NewProjectPath;
    ObservableProperty<int> SelectedTemplateIndex{0};
    ObservableProperty<int> SelectedRecentIndex{-1};
    ObservableProperty<std::vector<ProjectData>> RecentProjects;
    ObservableProperty<std::vector<std::shared_ptr<ProjectTemplate>>> Templates;
    ObservableProperty<std::string> StatusMessage{""};
    ObservableProperty<bool> IsLoading{false};
    ObservableProperty<std::shared_ptr<Project>> LoadedProject{nullptr};

    // Commands
    std::unique_ptr<RelayCommand<>> CreateProjectCommand;
    std::unique_ptr<RelayCommand<int>> OpenProjectCommand;
    std::unique_ptr<RelayCommand<int>> RemoveRecentCommand;
    std::unique_ptr<RelayCommand<>> RefreshCommand;
    std::unique_ptr<RelayCommand<>> BrowsePathCommand;
    std::unique_ptr<RelayCommand<>> SwitchToCreateCommand;
    std::unique_ptr<RelayCommand<>> SwitchToOpenCommand;

    ProjectBrowserViewModel()
    {
        InitializeCommands();
        InitializeDefaults();
        LoadTemplates();
        LoadRecentProjects();
    }


    bool ShouldCloseWindow() const { return LoadedProject.Get() != nullptr; }

private:
    fs::path m_appDataPath;
    fs::path m_projectDataPath;

    void InitializeCommands()
    {
        CreateProjectCommand = std::make_unique<RelayCommand<>>(
            [this]() { ExecuteCreateProject(); },
            [this]() { return ValidateNewProject(); }
        );

        OpenProjectCommand = std::make_unique<RelayCommand<int>>(
            [this](int index) { ExecuteOpenProject(index); },
            [this](int index) {
                auto projects = RecentProjects.Get();
                return index >= 0 && index < projects.size() &&
                       fs::exists(projects[index].GetFullPath());
            }
        );

        RemoveRecentCommand = std::make_unique<RelayCommand<int>>(
            [this](int index) { ExecuteRemoveRecent(index); },
            [this](int index) {
                return index >= 0 && index < RecentProjects.Get().size();
            }
        );

        RefreshCommand = std::make_unique<RelayCommand<>>(
            [this]() {
                LoadRecentProjects();
                UpdateStatus("Refreshed project list");
            }
        );

        BrowsePathCommand = std::make_unique<RelayCommand<>>(
            [this]() { ExecuteBrowsePath(); }
        );

        SwitchToCreateCommand = std::make_unique<RelayCommand<>>(
            [this]() {
                IsCreatingNew = true;
                SelectedRecentIndex = -1;
            }
        );

        SwitchToOpenCommand = std::make_unique<RelayCommand<>>(
            [this]() {
                IsCreatingNew = false;
            }
        );
    }

    void InitializeDefaults()
    {
#ifdef _WIN32
        NewProjectPath = fs::path(std::getenv("USERPROFILE")) / "Documents" / "Lark";
#else
        NewProjectPath = fs::path(std::getenv("HOME")) / "Documents" / "Lark";
#endif

        m_appDataPath = Utils::GetApplicationDataPath();
        m_projectDataPath = m_appDataPath / "ProjectData.xml";

        if (!fs::exists(m_appDataPath))
        {
            fs::create_directories(m_appDataPath);
        }
    }

    void LoadTemplates()
    {
        std::string enginePath = Utils::GetEnvVar("LARK_ENGINE");
        if (enginePath.empty())
        {
            Utils::s_showEnginePathPopup = true;
            UpdateStatus("Engine path not set");
            return;
        }

        fs::path templatePath = Utils::GetEngineResourcePath();
        Templates = ProjectTemplate::LoadTemplates(templatePath);

        if (Templates.Get().empty())
        {
            Logger::Get().Log(MessageType::Warning,
                            "No templates found in: " + templatePath.string());
            UpdateStatus("No templates found");
        }
        else
        {
            Logger::Get().Log(MessageType::Info,
                            "Loaded " + std::to_string(Templates.Get().size()) + " templates");
        }
    }

    void LoadRecentProjects()
    {
        std::vector<ProjectData> projects;

        if (!fs::exists(m_projectDataPath))
        {
            Logger::Get().Log(MessageType::Info, "No project data file found");
            RecentProjects = projects;
            return;
        }

        try
        {
            tinyxml2::XMLDocument doc;
            if (doc.LoadFile(m_projectDataPath.string().c_str()) != tinyxml2::XML_SUCCESS)
            {
                Logger::Get().Log(MessageType::Error,
                                "Failed to load project data file: " + m_projectDataPath.string());
                RecentProjects = projects;
                return;
            }

            auto root = doc.FirstChildElement("ProjectDataList");
            if (!root)
            {
                Logger::Get().Log(MessageType::Error, "No ProjectDataList element found");
                RecentProjects = projects;
                return;
            }

            auto projectsElement = root->FirstChildElement("Projects");
            if (!projectsElement)
            {
                Logger::Get().Log(MessageType::Error, "No Projects element found");
                RecentProjects = projects;
                return;
            }

            SerializationContext context(doc);

            for (auto element = projectsElement->FirstChildElement("ProjectData");
                 element;
                 element = element->NextSiblingElement("ProjectData"))
            {
                ProjectData data;
                if (data.Deserialize(element, context))
                {
                    // Only add if the project file exists
                    if (fs::exists(data.GetFullPath()))
                    {
                        projects.push_back(data);
                    }
                    else
                    {
                        Logger::Get().Log(MessageType::Warning,
                                        "Project file not found: " + data.GetFullPath().string());
                    }
                }
            }

            // Sort by date (newest first)
            std::sort(projects.begin(), projects.end(),
                     [](const ProjectData& a, const ProjectData& b) {
                         return a.date > b.date;
                     });

            RecentProjects = projects;
            Logger::Get().Log(MessageType::Info,
                            "Loaded " + std::to_string(projects.size()) + " recent projects");
        }
        catch (const std::exception& e)
        {
            Logger::Get().Log(MessageType::Error,
                            "Error loading recent projects: " + std::string(e.what()));
            RecentProjects = projects;
        }
    }

    bool ValidateNewProject()
    {
        if (NewProjectName.Get().empty())
        {
            UpdateStatus("Project name cannot be empty");
            return false;
        }

        if (!fs::exists(NewProjectPath.Get()))
        {
            try
            {
                fs::create_directories(NewProjectPath.Get());
            }
            catch (const std::exception& e)
            {
                UpdateStatus("Failed to create directory: " + std::string(e.what()));
                return false;
            }
        }

        fs::path fullPath = NewProjectPath.Get() / NewProjectName.Get();
        if (fs::exists(fullPath))
        {
            UpdateStatus("Project already exists at this location");
            return false;
        }

        return true;
    }

    void ExecuteCreateProject()
    {
        if (!ValidateNewProject()) return;

        auto templates = Templates.Get();
        if (SelectedTemplateIndex.Get() >= templates.size())
        {
            UpdateStatus("Invalid template selected");
            return;
        }

        IsLoading = true;
        UpdateStatus("Creating project...");

        auto tmpl = templates[SelectedTemplateIndex.Get()];

        // Use existing Project::Create method
        auto project = Project::Create(NewProjectName.Get(), NewProjectPath.Get(), *tmpl);

        if (project)
        {
            // Add to recent projects
            ProjectData projectData;
            projectData.name = NewProjectName.Get();
            projectData.path = NewProjectPath.Get() / NewProjectName.Get();

            // Get current time
            auto now = std::chrono::system_clock::now();
            auto timeT = std::chrono::system_clock::to_time_t(now);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S");
            projectData.date = ss.str();

            AddToRecentProjects(projectData);

            LoadedProject = project;
            UpdateStatus("Project created successfully");
            Logger::Get().Log(MessageType::Info, "Project created: " + project->GetName());
        }
        else
        {
            UpdateStatus("Failed to create project");
            Logger::Get().Log(MessageType::Error, "Failed to create project");
        }

        IsLoading = false;
    }

    void ExecuteOpenProject(int index)
    {
        auto projects = RecentProjects.Get();
        if (index >= projects.size()) return;

        IsLoading = true;
        UpdateStatus("Loading project...");

        const auto& projectData = projects[index];

        // Use existing Project::Load method
        auto project = Project::Load(projectData.GetFullPath());

        if (project)
        {
            // Update last opened time
            projects[index].date = GetCurrentTimeString();

            // Move to top of list
            std::rotate(projects.begin(), projects.begin() + index, projects.begin() + index + 1);

            RecentProjects = projects;
            SaveRecentProjects();

            LoadedProject = project;
            UpdateStatus("Project loaded successfully");
            Logger::Get().Log(MessageType::Info, "Project loaded: " + project->GetName());
        }
        else
        {
            UpdateStatus("Failed to load project");
            Logger::Get().Log(MessageType::Error,
                            "Failed to load project: " + projectData.GetFullPath().string());
        }

        IsLoading = false;
    }

    void ExecuteRemoveRecent(int index)
    {
        auto projects = RecentProjects.Get();
        if (index < projects.size())
        {
            std::string name = projects[index].name;
            projects.erase(projects.begin() + index);
            RecentProjects = projects;
            SaveRecentProjects();

            if (SelectedRecentIndex.Get() >= projects.size())
            {
                SelectedRecentIndex = projects.empty() ? -1 : 0;
            }

            UpdateStatus("Removed from recent projects: " + name);
            Logger::Get().Log(MessageType::Info, "Removed from recent: " + name);
        }
    }

    void ExecuteBrowsePath()
    {
        // This would open a native file dialog
        // For now, just log
        Logger::Get().Log(MessageType::Info, "Browse path clicked");
    }

    void AddToRecentProjects(const ProjectData& projectData)
    {
        auto projects = RecentProjects.Get();

        // Remove if already exists
        auto it = std::remove_if(projects.begin(), projects.end(),
                                [&](const ProjectData& p) {
                                    return p.path == projectData.path && p.name == projectData.name;
                                });
        projects.erase(it, projects.end());

        // Add to front
        projects.insert(projects.begin(), projectData);

        // Limit to max recent projects (e.g., 10)
        const size_t MAX_RECENT = 10;
        if (projects.size() > MAX_RECENT)
        {
            projects.resize(MAX_RECENT);
        }

        RecentProjects = projects;
        SaveRecentProjects();
    }

    void SaveRecentProjects()
    {
        try
        {
            tinyxml2::XMLDocument doc;
            SerializationContext context(doc);

            auto decl = doc.NewDeclaration();
            doc.LinkEndChild(decl);

            auto root = doc.NewElement("ProjectDataList");
            root->SetAttribute("xmlns",
                             "http://schemas.datacontract.org/2004/07/LarkEditor.lark");
            root->SetAttribute("xmlns:i", "http://www.w3.org/2001/XMLSchema-instance");
            doc.LinkEndChild(root);

            auto projectsElement = doc.NewElement("Projects");
            root->LinkEndChild(projectsElement);

            for (const auto& project : RecentProjects.Get())
            {
                if (project.name.empty() || project.path.empty())
                    continue;

                auto projectElement = doc.NewElement("ProjectData");
                project.Serialize(projectElement, context);
                projectsElement->LinkEndChild(projectElement);
            }

            if (doc.SaveFile(m_projectDataPath.string().c_str()) == tinyxml2::XML_SUCCESS)
            {
                Logger::Get().Log(MessageType::Info, "Saved recent projects list");
            }
            else
            {
                Logger::Get().Log(MessageType::Error, "Failed to save recent projects list");
            }
        }
        catch (const std::exception& e)
        {
            Logger::Get().Log(MessageType::Error,
                            "Error saving recent projects: " + std::string(e.what()));
        }
    }

    std::string GetCurrentTimeString()
    {
        auto now = std::chrono::system_clock::now();
        auto timeT = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&timeT), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    void UpdateStatus(const std::string& message)
    {
        StatusMessage = message;
    }
};