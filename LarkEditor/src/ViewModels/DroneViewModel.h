#pragma once
#include "ObservableProperty.h"
#include "../Commands/ICommand.h"
#include "../Services/EventBus.h"
#include "../Services/SelectionService.h"
#include "../Project/Project.h"
#include "../Components/Drone.h"
#include "../Components/Physics.h"
#include "../Utils/Etc/Logger.h"
#include <memory>

class DroneViewModel
{
public:
    // Observable properties
    ObservableProperty<uint32_t> SelectedEntityId{static_cast<uint32_t>(-1)};
    ObservableProperty<bool> HasSelection{false};
    ObservableProperty<bool> HasDrone{false};
    ObservableProperty<bool> HasPhysics{false};
    ObservableProperty<std::string> StatusMessage{""};

    // Drone Params
    ObservableProperty<float> Mass{1.0f};
    ObservableProperty<float> ArmLength{0.25f};
    ObservableProperty<float> RotorRadius{0.1f};
    ObservableProperty<control_abstraction> ControlAbstraction{control_abstraction::CMD_VEL};
    ObservableProperty<trajectory_type> TrajectoryType{trajectory_type::Circular};
    ObservableProperty<float> TrajectoryRadius{1.0f};
    ObservableProperty<float> TrajectoryFrequency{0.5f};

    // Drone State
    ObservableProperty<glm::vec3> DronePosition{0.f, 0.f, 0.f};
    ObservableProperty<glm::vec3> DroneVelocity{0.f, 0.f, 0.f};
    ObservableProperty<glm::vec4> DroneAttitude{0.f, 0.f, 0.f, 0.f};

    // Commands
    std::unique_ptr<RelayCommand<>> AddDroneCommand;
    std::unique_ptr<RelayCommand<>> RemoveDroneCommand;
    std::unique_ptr<RelayCommand<>> UpdateParametersCommand;
    std::unique_ptr<RelayCommand<control_abstraction>> SetControlAbstractionCommand;
    std::unique_ptr<RelayCommand<trajectory_type>> SetTrajectoryCommand;
    std::unique_ptr<RelayCommand<>> RefreshCommand;

    DroneViewModel()
    {
        InitializeCommands();
        SubscribeToSelectionService();
        SubscribeToEvents();
    }

    void SetProject(std::shared_ptr<Project> project)
    {
        if (m_project != project) {
            m_project = project;
            RefreshSelection();
        }
    }

private:
    std::shared_ptr<Project> m_project;
    std::shared_ptr<GameEntity> m_selectedEntity;

    void InitializeCommands()
    {
        AddDroneCommand = std::make_unique<RelayCommand<>>(
            [this]() { ExecuteAddDrone(); },
            [this]() { return HasSelection.Get() && !HasDrone.Get() && HasPhysics.Get(); }
        );

        RemoveDroneCommand = std::make_unique<RelayCommand<>>(
            [this]() { ExecuteRemoveDrone(); },
            [this]() { return HasDrone.Get(); }
        );

        UpdateParametersCommand = std::make_unique<RelayCommand<>>(
            [this]() { ExecuteUpdateParameters(); },
            [this]() { return HasDrone.Get(); }
        );

        SetControlAbstractionCommand = std::make_unique<RelayCommand<control_abstraction>>(
            [this](control_abstraction ca) { ExecuteSetControlAbstraction(ca); },
            [this](control_abstraction) { return HasDrone.Get(); }
        );

        SetTrajectoryCommand = std::make_unique<RelayCommand<trajectory_type>>(
            [this](trajectory_type type) { ExecuteSetTrajectory(type); },
            [this](trajectory_type) { return HasDrone.Get(); }
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
        if (!m_project || !m_project->GetActiveScene()) {
            ClearSelection();
            return;
        }

        auto scene = m_project->GetActiveScene();
        auto entity = scene->GetEntity(entityId);

        if (!entity) {
            ClearSelection();
            return;
        }

        m_selectedEntity = entity;
        SelectedEntityId = entityId;
        HasSelection = true;

        // Check for physics (required for drone)
        HasPhysics = (entity->GetComponent<Physics>() != nullptr);

        // Check for drone
        if (auto* drone = entity->GetComponent<Drone>()) {
            HasDrone = true;
            LoadDroneParameters(drone);
        } else {
            HasDrone = false;
            SetDefaultParameters();
        }
    }

    void ClearSelection()
    {
        m_selectedEntity.reset();
        SelectedEntityId = static_cast<uint32_t>(-1);
        HasSelection = false;
        HasDrone = false;
        HasPhysics = false;
    }

    void RefreshSelection()
    {
        if (HasSelection.Get()) {
            HandleSelectionChanged(SelectedEntityId.Get());
        }
    }

    void LoadDroneParameters(Drone* drone)
    {
        const auto& params = drone->GetParams();
        Mass = params.i.mass;
        ArmLength = glm::length(params.g.rotor_positions[0]);
        RotorRadius = params.g.rotor_radius;
        ControlAbstraction = drone->GetControlAbstraction();

        const auto& trajectory = drone->GetTrajectory();
        TrajectoryType = trajectory.type;
        TrajectoryRadius = trajectory.radius;
        TrajectoryFrequency = trajectory.frequency;

        const auto& state = drone->GetDroneState();
        DronePosition = state.position;
        DroneVelocity = state.velocity;
        DroneAttitude = state.attitude;
    }

    void SetDefaultParameters()
    {
        Mass = 1.0f;
        ArmLength = 0.25f;
        RotorRadius = 0.1f;
        ControlAbstraction = control_abstraction::CMD_VEL;
        TrajectoryType = trajectory_type::Circular;
        TrajectoryRadius = 1.0f;
        TrajectoryFrequency = 0.5f;
    }

    void ExecuteAddDrone()
    {
        if (!m_selectedEntity || !HasPhysics.Get()) return;

        DroneInitializer droneInit;
        droneInit.params = CreateQuadParams();
        droneInit.control_abstraction = ControlAbstraction.Get();
        droneInit.trajectory = CreateTrajectory();
        droneInit.drone_state = CreateInitialDroneState();
        droneInit.input = control_input{};

        if (auto* drone = m_selectedEntity->AddComponent<Drone>(&droneInit)) {
            HasDrone = true;

            // Update entity in scene
            if (m_project && m_project->GetActiveScene()) {
                m_project->GetActiveScene()->UpdateEntity(m_selectedEntity->GetID());
            }

            UpdateStatus("Drone component added");
            Logger::Get().Log(MessageType::Info, "Added drone component");
        }
    }

    void ExecuteRemoveDrone()
    {
        if (!m_selectedEntity) return;

        if (m_selectedEntity->RemoveComponent<Drone>()) {
            HasDrone = false;

            // Update entity in scene
            if (m_project && m_project->GetActiveScene()) {
                m_project->GetActiveScene()->UpdateEntity(m_selectedEntity->GetID());
            }

            UpdateStatus("Drone component removed");
            Logger::Get().Log(MessageType::Info, "Removed drone component");
        }
    }

    void ExecuteUpdateParameters()
    {
        if (!m_selectedEntity || !HasDrone.Get()) return;

        if (auto* drone = m_selectedEntity->GetComponent<Drone>()) {
            drone->GetParams() = CreateQuadParams();

            // Update entity in scene
            if (m_project && m_project->GetActiveScene()) {
                m_project->GetActiveScene()->UpdateEntity(m_selectedEntity->GetID());
            }

            UpdateStatus("Drone parameters updated");
        }
    }

    void ExecuteSetControlAbstraction(control_abstraction ca)
    {
        if (!m_selectedEntity || !HasDrone.Get()) return;

        if (auto* drone = m_selectedEntity->GetComponent<Drone>()) {
            drone->SetControlAbstraction(ca);
            ControlAbstraction = ca;

            // Update entity in scene
            if (m_project && m_project->GetActiveScene()) {
                m_project->GetActiveScene()->UpdateEntity(m_selectedEntity->GetID());
            }

            UpdateStatus("Control abstraction updated");
        }
    }

    void ExecuteSetTrajectory(trajectory_type type)
    {
        if (!m_selectedEntity || !HasDrone.Get()) return;

        if (auto* drone = m_selectedEntity->GetComponent<Drone>()) {
            drone->GetTrajectory() = CreateTrajectory();
            TrajectoryType = type;

            // Update entity in scene
            if (m_project && m_project->GetActiveScene()) {
                m_project->GetActiveScene()->UpdateEntity(m_selectedEntity->GetID());
            }

            UpdateStatus("Trajectory updated");
        }
    }

    quad_params CreateQuadParams()
    {
        quad_params params;

        // Inertia properties
        params.i.mass = Mass.Get();
        params.i.principal_inertia = glm::vec3(0.0023f, 0.0023f, 0.004f);
        params.i.product_inertia = glm::vec3(0.0f);

        // Geometry properties
        float arm = ArmLength.Get();
        params.g.rotor_radius = RotorRadius.Get();
        params.g.rotor_positions[0] = glm::vec3(arm, 0, 0);
        params.g.rotor_positions[1] = glm::vec3(0, arm, 0);
        params.g.rotor_positions[2] = glm::vec3(-arm, 0, 0);
        params.g.rotor_positions[3] = glm::vec3(0, -arm, 0);
        params.g.rotor_directions = glm::vec4(1, -1, 1, -1);
        params.g.imu_positions = glm::vec3(0, 0, 0);

        // Aerodynamics
        params.a.parasitic_drag = glm::vec3(0.2f);

        // Rotor properties
        params.r.k_eta = 1e-3f;
        params.r.k_m = 2.5e-2f;
        params.r.k_d = 0.0f;
        params.r.k_z = 0.0f;
        params.r.k_h = 0.0f;
        params.r.k_flap = 0.0f;

        // Motor properties
        params.m.tau_m = 0.02f;
        params.m.rotor_speed_min = 0.0f;
        params.m.rotor_speed_max = 2500.0f;
        params.m.motor_noise_std = 0.0f;

        // Control gains
        params.c.kp_pos = glm::vec3(6.5f, 6.5f, 15.0f);
        params.c.kd_pos = glm::vec3(4.0f, 4.0f, 9.0f);
        params.c.kp_att = 544.0f;
        params.c.kd_att = 46.64f;
        params.c.kp_vel = glm::vec3(0.65f, 0.65f, 1.5f);

        // Lower level controller
        params.l.k_w = 0.18f;
        params.l.k_v = 0.18f;
        params.l.kp_att = 70000;
        params.l.kd_att = 7000.0f;

        return params;
    }

    trajectory CreateTrajectory()
    {
        trajectory traj;
        traj.type = TrajectoryType.Get();
        traj.position = glm::vec3(0.0f);
        traj.radius = TrajectoryRadius.Get();
        traj.frequency = TrajectoryFrequency.Get();
        traj.delta = 1.0f;
        traj.n_points = 10;
        traj.segment_time = 1.0f;
        return traj;
    }

    drone_state CreateInitialDroneState()
    {
        drone_state state;
        state.position = DronePosition.Get();
        state.velocity = DroneVelocity.Get();
        state.attitude = DroneAttitude.Get();
        state.body_rates = glm::vec3(0.0f);
        state.wind = glm::vec3(0.0f);
        state.rotor_speeds = glm::vec4(0.0f);
        return state;
    }

    void UpdateStatus(const std::string& message)
    {
        StatusMessage = message;
    }
};