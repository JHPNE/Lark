#pragma once
#include "../Project/Project.h"
#include "../Project/ProjectTemplate.h"
#include "../Project/ProjectData.h"
#include <memory>
#include <vector>

class ProjectBrowserView {
public:
    static ProjectBrowserView& Get() {
        static ProjectBrowserView instance;
        return instance;
    }

    void Draw();
    bool& GetShowState() { return m_show; }
    std::shared_ptr<Project> GetLoadedProject() const { return m_loadedProject; }
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

    // Enhanced project management
    void LoadRecentProjects();
    bool ReadProjectData();
    bool WriteProjectData();
    void OpenSelectedProject();
    bool PromptSaveChanges(); // For handling unsaved changes
    void UpdateRecentProject(const ProjectData& projectData);
    void RemoveRecentProject(size_t index);

    // UI state
    bool m_show = false;
    bool m_isNewProject = true;  // false = open project
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
    static constexpr const char* PROJECT_DATA_FILENAME = "ProjectData.xml";

    // Helper methods for project management
    bool EnsureProjectDirectoryExists(const fs::path& path);
    fs::path GetUniqueProjectPath(const fs::path& basePath, const std::string& projectName);
    void CleanupOldProjects(); // Remove non-existent projects from recent list
};