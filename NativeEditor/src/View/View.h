// View.h
#pragma once
#include <memory>

class Project; // Forward declaration

class View {
public:
  virtual ~View() = default;

  // Core functionality
  virtual void Draw() = 0;

  // Show/hide management
  bool& GetShowState() { return m_show; }
  void Show() { m_show = true; }
  void Hide() { m_show = false; }
  bool IsVisible() const { return m_show; }

  // Project management
  virtual void SetActiveProject(std::shared_ptr<Project> activeProject) {
    project = activeProject;
  }

  std::shared_ptr<Project> GetActiveProject() const {
    return project;
  }

protected:
  View() = default;

  bool m_show = true;
  std::shared_ptr<Project> project;
};