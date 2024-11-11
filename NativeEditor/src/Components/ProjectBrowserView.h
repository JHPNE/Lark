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

private:
    ProjectBrowserView() { 
        LoadTemplates(); 
        LoadRecentProjects();
    }

    void DrawNewProject();
    void DrawOpenProject();
    void LoadTemplates();
    bool ValidateProjectPath();

	void LoadRecentProjects();
	bool ReadProjectData();
	bool WriteProjectData();

    bool m_show = false;
    bool m_isNewProject = true;  // false = open project
    std::string m_newProjectName = "NewProject";
#ifdef _WIN32
    fs::path m_projectPath = fs::path(std::getenv("USERPROFILE")) / "Documents" / "Drosim";
#else
    fs::path m_projectPath = fs::path(std::getenv("HOME")) / "Documents" / "Drosim";
#endif
    std::vector<std::shared_ptr<ProjectTemplate>> m_templates;
    int m_selectedTemplate = 0;

	std::vector<ProjectData> m_recentProjects;
	int m_selectedRecentProject = -1;
	fs::path m_appDataPath;
    fs::path m_projectDataPath;
	std::shared_ptr<Project> m_loadedProject;
};