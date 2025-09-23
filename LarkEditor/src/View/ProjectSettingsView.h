// LarkEditor/src/View/ProjectSettingsView.h
#pragma once
#include <memory>

class Project;
class ProjectSettingsViewModel;

class ProjectSettingsView
{
public:
    static ProjectSettingsView& Get()
    {
        static ProjectSettingsView instance;
        return instance;
    }

    void Draw();
    bool& GetShowState() { return m_show; }
    void SetActiveProject(std::shared_ptr<Project> activeProject);

    // Get the view model for external access (e.g., for camera settings)
    ProjectSettingsViewModel* GetViewModel() { return m_viewModel.get(); }

private:
    ProjectSettingsView();
    ~ProjectSettingsView();

    void DrawCameraTab();
    void DrawGeometryTab();
    void DrawWorldTab();
    void DrawRenderTab();

    bool m_show = true;
    bool m_showFileDialog = false;
    std::unique_ptr<ProjectSettingsViewModel> m_viewModel;
};