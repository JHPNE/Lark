#pragma once
#include "ObservableProperty.h"
#include "../Commands/ICommand.h"
#include <Services/EventBus.h>

#include "Project/Project.h"
#include "Services/PhysicService.h"
#include "Services/SelectionService.h"

class PhysicsViewModel {
public:
    // Observable properties for UI
    ObservableProperty<uint32_t> SelectedEntityId{static_cast<uint32_t>(-1)};
    ObservableProperty<bool> HasSelection{false};
    ObservableProperty<bool> HasPhysics{false};
    ObservableProperty<bool> HasGeometry{false};
    ObservableProperty<std::string> StatusMessage{""};

    // Physics Properties
    ObservableProperty<float> Mass{1.0f};
    ObservableProperty<glm::vec3> Inertia{1.0f, 1.0f, 1.0f};
    ObservableProperty<bool> IsKinematic{false};

    // World Settings
    ObservableProperty<glm::vec3> Gravity{0.0f, -9.81f, 0.0f};
    ObservableProperty<wind_type> WindType{wind_type::NoWind};
    ObservableProperty<glm::vec3> WindVector{0.0f, 0.0f, 0.0f};
    ObservableProperty<glm::vec3> WindAmplitudes{1.0f, 1.0f, 1.0f};
    ObservableProperty<glm::vec3> WindFrequencies{1.0f, 1.0f, 1.0f};

    // Commands
    std::unique_ptr<RelayCommand<>> AddPhysicsCommand;
    std::unique_ptr<RelayCommand<>> RemovePhysicsCommand;
    std::unique_ptr<RelayCommand<float>> UpdateMassCommand;
    std::unique_ptr<RelayCommand<glm::vec3>> UpdateInertiaCommand;
    std::unique_ptr<RelayCommand<bool>> SetKinematicCommand;
    std::unique_ptr<RelayCommand<>> ApplyWorldSettingsCommand;
    std::unique_ptr<RelayCommand<>> RefreshCommand;

    PhysicsViewModel()
        : m_physicService(PhysicService::Get())
    {
        InitializeCommands();
        SubscribeToSelectionService();
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
    PhysicService& m_physicService;
    std::shared_ptr<GameEntity> m_selectedEntity;

    void InitializeCommands()
    {
        AddPhysicsCommand = std::make_unique<RelayCommand<>>(
            [this]() { ExecuteAddPhysics(); },
            [this]() { return HasSelection.Get() && !HasPhysics.Get() && HasGeometry.Get(); }
        );

        RemovePhysicsCommand = std::make_unique<RelayCommand<>>(
            [this]() { ExecuteRemovePhysics(); },
            [this]() { return HasPhysics.Get(); }
        );

        UpdateMassCommand = std::make_unique<RelayCommand<float>>(
            [this](float mass) { ExecuteUpdateMass(mass); },
            [this](float) { return HasPhysics.Get(); }
        );

        UpdateInertiaCommand = std::make_unique<RelayCommand<glm::vec3>>(
            [this](const glm::vec3& inertia) { ExecuteUpdateInertia(inertia); },
            [this](const glm::vec3&) { return HasPhysics.Get(); }
        );

        SetKinematicCommand = std::make_unique<RelayCommand<bool>>(
            [this](bool kinematic) { ExecuteSetKinematic(kinematic); },
            [this](bool) { return HasPhysics.Get(); }
        );

        RefreshCommand = std::make_unique<RelayCommand<>>(
            [this]() { RefreshSelection(); }
        );
    }

    void SubscribeToSelectionService()
    {
        auto& selectionService = SelectionService::Get();

        selectionService.SubscribeToSelectionChange(
            [this](uint32_t oldId, uint32_t newId) {
                HandleSelectionChanged(newId);
            }
        );
    }

    void SubscribeToEvents()
    {
        EventBus::Get().Subscribe<EntityRemovedEvent>(
            [this](const EntityRemovedEvent& e) {
                if (e.entityId == SelectedEntityId.Get()) {
                    ClearSelection();
                }
            }
        );

        EventBus::Get().Subscribe<SceneChangedEvent>(
            [this](const SceneChangedEvent& e) {
                RefreshSelection();
            }
        );
    }

    void HandleSelectionChanged(uint32_t entityId)
    {
        if (!m_project || !m_project->GetActiveScene())
        {
            ClearSelection();
            return;
        }

        auto scene = m_project->GetActiveScene();
        auto entity = scene->GetEntity(entityId);

        if (!entity)
        {
            ClearSelection();
            return;
        }

        m_selectedEntity = entity;
        SelectedEntityId = entityId;
        HasSelection = true;

        // Check for geometry (required for physics)
        HasGeometry = (entity->GetComponent<Geometry>() != nullptr);

        // Check for physics
        if (auto* physics = entity->GetComponent<Physics>())
        {
            HasPhysics = true;
            Mass = physics->GetMass();
            Inertia = physics->GetInertia();
            IsKinematic = physics->IsKinematic();
        }
        else
        {
            HasPhysics = false;
            Mass = 1.0f;
            Inertia = glm::vec3(1.0f);
            IsKinematic = false;
        }
    }

    void ClearSelection()
    {
        m_selectedEntity.reset();
        SelectedEntityId = static_cast<uint32_t>(-1);
        HasSelection = false;
        HasPhysics = false;
        HasGeometry = false;
    }

    void RefreshSelection()
    {
        if (HasSelection.Get()) HandleSelectionChanged(SelectedEntityId.Get());
    }

    void ExecuteAddPhysics()
    {
        if (!m_selectedEntity || !HasGeometry.Get()) return;

        PhysicInitializer physicInit;
        physicInit.mass = Mass.Get();
        physicInit.inertia = Inertia.Get();
        physicInit.is_kinematic = IsKinematic.Get();

        if (auto* physics = m_selectedEntity->AddComponent<Physics>(&physicInit))
        {
            HasPhysics = true;

            if (m_project && m_project->GetActiveScene()) m_project->GetActiveScene()->UpdateEntity(m_selectedEntity->GetID());

            UpdateStatus("Physics component added");
            Logger::Get().Log(MessageType::Info, "Added physics component");
        }
    }

    void ExecuteRemovePhysics()
    {
        if (!m_selectedEntity) return;

        // Check if drone component exists (it depends on physics)
        if (m_selectedEntity->GetComponent<Drone>()) {
            UpdateStatus("Cannot remove physics while drone component exists");
            Logger::Get().Log(MessageType::Warning, "Remove drone component first");
            return;
        }

        if (m_selectedEntity->RemoveComponent<Physics>()) {
            HasPhysics = false;

            // Update entity in scene
            if (m_project && m_project->GetActiveScene()) {
                m_project->GetActiveScene()->UpdateEntity(m_selectedEntity->GetID());
            }

            UpdateStatus("Physics component removed");
            Logger::Get().Log(MessageType::Info, "Removed physics component");
        }
    }


    void ExecuteUpdateMass(float mass)
    {
        if (!m_selectedEntity || !HasPhysics.Get()) return;

        if (auto* physics = m_selectedEntity->GetComponent<Physics>()) {
            physics->SetMass(mass);
            Mass = mass;

            // Update entity in scene
            if (m_project && m_project->GetActiveScene()) {
                m_project->GetActiveScene()->UpdateEntity(m_selectedEntity->GetID());
            }

            UpdateStatus("Mass updated");
        }
    }

    void ExecuteUpdateInertia(const glm::vec3& inertia)
    {
        if (!m_selectedEntity || !HasPhysics.Get()) return;

        if (auto* physics = m_selectedEntity->GetComponent<Physics>()) {
            physics->SetInertia(inertia);
            Inertia = inertia;

            // Update entity in scene
            if (m_project && m_project->GetActiveScene()) {
                m_project->GetActiveScene()->UpdateEntity(m_selectedEntity->GetID());
            }

            UpdateStatus("Inertia updated");
        }
    }

    void ExecuteSetKinematic(bool kinematic)
    {
        if (!m_selectedEntity || !HasPhysics.Get()) return;

        if (auto* physics = m_selectedEntity->GetComponent<Physics>()) {
            physics->SetKinematic(kinematic);
            IsKinematic = kinematic;

            // Update entity in scene
            if (m_project && m_project->GetActiveScene()) {
                m_project->GetActiveScene()->UpdateEntity(m_selectedEntity->GetID());
            }

            UpdateStatus(kinematic ? "Set to kinematic" : "Set to dynamic");
        }
    }

    void UpdateStatus(const std::string& message)
    {
        StatusMessage = message;
    }

};
