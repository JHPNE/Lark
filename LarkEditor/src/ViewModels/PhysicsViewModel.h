#pragma once
#include "ObservableProperty.h"
#include "../Commands/ICommand.h"
#include <Services/EventBus.h>

#include "Project/Project.h"

class PhysicsViewModel
{
public:
    // Observable properties for UI
    ObservableProperty<uint32_t> SelectedEntityId{static_cast<uint32_t>(-1)};
    ObservableProperty<bool> HasSelection{false};
    ObservableProperty<std::string> StatusMessage{""};

    PhysicsViewModel()
    {
        InitializeCommands();
        SubscribeToEvents();
    }

    ~PhysicsViewModel() = default;

    void SetProject(std::shared_ptr<Project> project)
    {
        if (m_project != project)
        {
            ClearAll();
            m_project = project;
        }
    }



private:
    std::shared_ptr<Project> m_project;

    void InitializeCommands()
    {

    }

    void SubscribeToEvents()
    {
        EventBus::Get().Subscribe<EntityRemovedEvent>(
            [this](const EntityRemovedEvent& e) {
            }
        );
    }

    void ClearAll()
    {

    }

};
