#pragma once
#include "../Project/Project.h"
#include "../Project/ProjectTemplate.h"
#include "../Project/ProjectData.h"
#include <memory>
#include <vector>
#include <filesystem>

class ProjectBrowserView {
public:
    static ProjectBrowserView& Get() {
        static ProjectBrowserView instance;
        return instance;
    }

    void Draw();
    bool& GetShowState() { return m_show; }
    [[nodiscard]] std::shared_ptr<Project> GetLoadedProject() const { return m_loadedProject; }
    void SetLoadedProject(const std::shared_ptr<Project>& project) { m_loadedProject = project; }

private:
    ProjectBrowserView() {
        LoadTemplates();
        LoadRecentProjects();
    }

    void DrawNewProject();
    void DrawOpenProject();
    void LoadTemplates();
    bool ValidateProjectPath();

    // Project management
    void LoadRecentProjects();
    bool ReadProjectData();
    bool WriteProjectData();
    void OpenSelectedProject();
    bool CreateNewProject();  // Added this method

    // UI state
    bool m_show = false;
    bool m_isNewProject = true;
    std::string m_newProjectName = "NewProject";
#ifdef _WIN32
    fs::path m_projectPath = fs::path(std::getenv("USERPROFILE")) / "Documents" / "Drosim";
#else
    fs::path m_projectPath = fs::path(std::getenv("HOME")) / "Documents" / "Drosim";
#endif

    // Project templates
    std::vector<std::shared_ptr<ProjectTemplate>> m_templates;
    int m_selectedTemplate = 0;

    // Recent projects
    std::vector<ProjectData> m_recentProjects;
    int m_selectedRecentProject = -1;
    fs::path m_appDataPath;
    fs::path m_projectDataPath;

    // Currently loaded project
    std::shared_ptr<Project> m_loadedProject;

    // Constants
    static constexpr size_t MAX_RECENT_PROJECTS = 10;
};