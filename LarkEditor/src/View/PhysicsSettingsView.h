#pragma once
#include "EngineAPI.h"
#include <memory>
#include <string>

class Project;

class PhysicsSettingsView
{
  public:
    static PhysicsSettingsView &Get()
    {
        static PhysicsSettingsView instance;
        return instance;
    }

    void Draw();
    void SetActiveProject(std::shared_ptr<Project> activeProject);
    bool &GetShowState() { return m_show; }

  private:
    PhysicsSettingsView() = default;
    ~PhysicsSettingsView() = default;

    // Prevent copying
    PhysicsSettingsView(const PhysicsSettingsView &) = delete;
    PhysicsSettingsView &operator=(const PhysicsSettingsView &) = delete;

    void CreateDrone();

    // UI State
    bool m_show = true;
    float m_arm_length = 0.25f;
    float m_mass = 1.0f;
    std::shared_ptr<Project> m_project;
};