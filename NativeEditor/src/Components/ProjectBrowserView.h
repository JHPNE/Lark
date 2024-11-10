#pragma once
#include "../Project/Project.h"
#include "../Project/ProjectTemplate.h"
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

private:
    ProjectBrowserView() { LoadTemplates(); }

    void DrawNewProject();
    void DrawOpenProject();
    void LoadTemplates();
    bool ValidateProjectPath();

    bool m_show = false;
    bool m_isNewProject = true;  // false = open project
    std::string m_newProjectName = "NewProject";
    fs::path m_projectPath = fs::current_path();
    std::vector<std::shared_ptr<ProjectTemplate>> m_templates;
    int m_selectedTemplate = 0;
};