#pragma once
#include "ObservableProperty.h"
#include "../Commands/ICommand.h"
#include "../Services/SelectionService.h"
#include "../Services/EventBus.h"
#include "../Project/Project.h"
#include "../Project/Scene.h"
#include "../Utils/Etc/Logger.h"

#include <imgui.h>
#include <memory>
#include <vector>

struct SceneNodeData
{
    uint32_t id;
    std::string name;
    bool isScene;
    bool isActive;
    bool isEnabled;
    bool isSelected;
    std::vector<SceneNodeData> children;

    bool operator==(const SceneNodeData& other) const
    {
        return id == other.id &&
               name == other.name &&
               isScene == other.isScene &&
               isActive == other.isActive &&
               isEnabled == other.isEnabled &&
               isSelected == other.isSelected &&
               children == other.children;
    }

    bool operator!=(const SceneNodeData& other) const
    {
        return !(*this == other);
    }
};

class SceneViewModel {
public:
    // Observable properties
    ObservableProperty<std::shared_ptr<Project>> CurrentProject{nullptr};
    ObservableProperty<std::shared_ptr<Scene>> ActiveScene{nullptr};
    ObservableProperty<uint32_t> SelectedEntityId{static_cast<uint32_t>(-1)};
    ObservableProperty<std::unordered_set<uint32_t>> SelectedEntityIds;
    ObservableProperty<bool> HasSelection{false};
    ObservableProperty<std::vector<SceneNodeData>> SceneHierarchy;

    // Commands
    std::unique_ptr<RelayCommand<>> AddSceneCommand;
    std::unique_ptr<RelayCommand<uint32_t>> RemoveSceneCommand;
    std::unique_ptr<RelayCommand<uint32_t>> SetActiveSceneCommand;
    std::unique_ptr<RelayCommand<>> AddEntityCommand;
    std::unique_ptr<RelayCommand<uint32_t>> RemoveEntityCommand;
    std::unique_ptr<RelayCommand<uint32_t>> SelectEntityCommand;
    std::unique_ptr<RelayCommand<uint32_t>> ToggleEntityEnabledCommand;

    SceneViewModel()
    {
        InitializeCommands();
        SubscribeToSelectionService();
        SubscribeToPropertyChanges();
        SubscribeToEvents();
    }

    void SetProject(std::shared_ptr<Project> project)
    {
        if (CurrentProject.Get() != project)
        {
            CurrentProject = project;
            if (project)
            {
                ActiveScene = project->GetActiveScene();
                RefreshHierarchy();
            }
            else
            {
                ActiveScene = std::shared_ptr<Scene>(nullptr);
                SceneHierarchy = std::vector<SceneNodeData>();
            }
        }
    }

    void RefreshHierarchy()
    {
        auto project = CurrentProject.Get();
        if (!project)
        {
            SceneHierarchy = std::vector<SceneNodeData>();
            return;
        }

        std::vector<SceneNodeData> hierarchy;
        auto& selectionService = SelectionService::Get();

        for (const auto& scene : project->GetScenes())
        {
            SceneNodeData sceneNode;
            sceneNode.id = scene->GetID();
            sceneNode.name = scene->GetName();
            sceneNode.isScene = true;
            sceneNode.isActive = (scene == project->GetActiveScene());
            sceneNode.isEnabled = true;
            sceneNode.isSelected = false;

            // Add entities as children
            for (const auto& entity : scene->GetEntities())
            {
                SceneNodeData entityNode;
                entityNode.id = entity->GetID();
                entityNode.name = entity->GetName();
                entityNode.isScene = false;
                entityNode.isActive = false;
                entityNode.isEnabled = entity->IsEnabled();
                entityNode.isSelected = selectionService.IsSelected(entity->GetID());

                sceneNode.children.push_back(entityNode);
            }

            hierarchy.push_back(sceneNode);
        }

        SceneHierarchy = hierarchy;
    }

private:
    void InitializeCommands()
    {
        AddSceneCommand = std::make_unique<RelayCommand<>>(
            [this]() { ExecuteAddScene(); },
            [this]() { return CurrentProject.Get() != nullptr; }
        );

        RemoveSceneCommand = std::make_unique<RelayCommand<uint32_t>>(
            [this](uint32_t sceneId) { ExecuteRemoveScene(sceneId); },
            [this](uint32_t) { return CurrentProject.Get() != nullptr; }
        );

        SetActiveSceneCommand = std::make_unique<RelayCommand<uint32_t>>(
            [this](uint32_t sceneId) { ExecuteSetActiveScene(sceneId); },
            [this](uint32_t) { return CurrentProject.Get() != nullptr; }
        );

        AddEntityCommand = std::make_unique<RelayCommand<>>(
            [this]() { ExecuteAddEntity(); },
            [this]() { return ActiveScene.Get() != nullptr; }
        );

        RemoveEntityCommand = std::make_unique<RelayCommand<uint32_t>>(
            [this](uint32_t entityId) { ExecuteRemoveEntity(entityId); },
            [this](uint32_t) { return ActiveScene.Get() != nullptr; }
        );

        SelectEntityCommand = std::make_unique<RelayCommand<uint32_t>>(
            [this](uint32_t entityId) { ExecuteSelectEntity(entityId); },
            [this](uint32_t) { return true; }
        );

        ToggleEntityEnabledCommand = std::make_unique<RelayCommand<uint32_t>>(
            [this](uint32_t entityId) { ExecuteToggleEntityEnabled(entityId); },
            [this](uint32_t) { return ActiveScene.Get() != nullptr; }
        );
    }

    void SubscribeToEvents()
    {
        EventBus::Get().Subscribe<EntityCreatedEvent>(
            [this](const EntityCreatedEvent& e) {
                RefreshHierarchy();
            }
        );

        EventBus::Get().Subscribe<EntityRemovedEvent>(
            [this](const EntityRemovedEvent& e) {
                RefreshHierarchy();
            }
        );

        EventBus::Get().Subscribe<SceneChangedEvent>(
            [this](const SceneChangedEvent& e) {
                RefreshHierarchy();
            }
        );
    }

    void SubscribeToSelectionService()
    {
        auto& selectionService = SelectionService::Get();

        selectionService.SubscribeToSelectionChange(
            [this](uint32_t oldId, uint32_t newId) {
                SelectedEntityId = newId;
                HasSelection = (newId != static_cast<uint32_t>(-1));

                // Update entity selection states in project
                if (CurrentProject.Get())
                {
                    for (const auto& scene : CurrentProject.Get()->GetScenes())
                    {
                        for (const auto& entity : scene->GetEntities())
                        {
                            entity->SetSelected(entity->GetID() == newId);
                        }
                    }
                }

                RefreshHierarchy();
            }
        );

        selectionService.SubscribeToMultiSelectionChange(
            [this](const std::unordered_set<uint32_t>& selectedIds) {
                SelectedEntityIds = selectedIds;
                HasSelection = !selectedIds.empty();

                // Update entity selection states
                if (CurrentProject.Get())
                {
                    for (const auto& scene : CurrentProject.Get()->GetScenes())
                    {
                        for (const auto& entity : scene->GetEntities())
                        {
                            bool isSelected = selectedIds.find(entity->GetID()) != selectedIds.end();
                            entity->SetSelected(isSelected);
                        }
                    }
                }

                RefreshHierarchy();
            }
        );
    }

    void SubscribeToPropertyChanges()
    {
        ActiveScene.Subscribe([this](const auto& old, const auto& newScene) {
            if (newScene && CurrentProject.Get())
            {
                CurrentProject.Get()->SetActiveScene(newScene->GetID());
            }
            RefreshHierarchy();
        });
    }

    void ExecuteAddScene()
    {
        static int sceneCounter = 0;
        std::string name = "Scene_" + std::to_string(++sceneCounter);

        if (auto project = CurrentProject.Get())
        {
            project->AddScene(name);
            RefreshHierarchy();
            Logger::Get().Log(MessageType::Info, "Added scene: " + name);
        }
    }

    void ExecuteRemoveScene(uint32_t sceneId)
    {
        if (auto project = CurrentProject.Get())
        {
            project->RemoveScene(sceneId);
            RefreshHierarchy();
            Logger::Get().Log(MessageType::Info, "Removed scene ID: " + std::to_string(sceneId));
        }
    }

    void ExecuteSetActiveScene(uint32_t sceneId)
    {
        if (auto project = CurrentProject.Get())
        {
            project->SetActiveScene(sceneId);
            ActiveScene = project->GetActiveScene();
            RefreshHierarchy();

            SceneChangedEvent event;
            event.sceneId = sceneId;
            EventBus::Get().Publish(event);
        }
    }

    void ExecuteAddEntity()
    {
        static int entityCounter = 0;
        std::string name = "Entity_" + std::to_string(++entityCounter);

        if (auto scene = ActiveScene.Get())
        {
            scene->CreateEntity(name);
            RefreshHierarchy();
            Logger::Get().Log(MessageType::Info, "Added entity: " + name);
        }
    }

    void ExecuteRemoveEntity(uint32_t entityId)
    {
        if (auto scene = ActiveScene.Get())
        {
            scene->RemoveEntity(entityId);
            SelectionService::Get().DeselectEntity(entityId);
            RefreshHierarchy();
            Logger::Get().Log(MessageType::Info, "Removed entity ID: " + std::to_string(entityId));

            EntityRemovedEvent event;
            event.entityId = entityId;
            event.sceneId = scene->GetID();
            EventBus::Get().Publish(event);
        }
    }

    void ExecuteSelectEntity(uint32_t entityId)
    {
        bool shiftHeld = ImGui::GetIO().KeyShift;
        SelectionService::Get().SelectEntity(entityId, shiftHeld);
    }

    void ExecuteToggleEntityEnabled(uint32_t entityId)
    {
        if (auto scene = ActiveScene.Get())
        {
            if (auto entity = scene->GetEntity(entityId))
            {
                entity->SetEnabled(!entity->IsEnabled());
                RefreshHierarchy();
            }
        }
    }
};