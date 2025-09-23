#pragma once
#include "ProjectBrowserView.h"
#include "../ViewModels/ProjectBrowserViewModel.h"
#include "Style/CustomWidgets.h"

class ProjectBrowserView
{
  public:
    static ProjectBrowserView &Get()
    {
        static ProjectBrowserView instance;
        return instance;
    }
    ProjectBrowserView();
    ~ProjectBrowserView();

    void Draw();
    void DrawBackground();

    void DrawLeftPanel(float width);
    void DrawRightPanel(float width);

    void DrawStatusBar();

    std::shared_ptr<Project> GetLoadedProject() { return m_loadedProject; }
    [[nodiscard]] bool ShouldTransition() const {return m_shouldTransition; }

  private:
    std::unique_ptr<ProjectBrowserViewModel> m_viewModel;
    std::shared_ptr<Project> m_loadedProject;
    bool m_shouldTransition = false;
};