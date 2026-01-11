#pragma once
#include "ObservableProperty.h"
#include "../Commands/ICommand.h"
#include "../Services/TransformService.h"
#include "../Services/SelectionService.h"
#include "../Services/EventBus.h"
#include "../Project/Project.h"
#include "../Components/Transform.h"
#include "../Components/Script.h"
#include "../Components/Geometry.h"
#include "../Components/Material.h"
#include "../Utils/Etc/Logger.h"
#include <memory>
#include <vector>
#include <unordered_set>

class ComponentViewModel
{
public:
    // Selection Properties
    ObservableProperty<std::shared_ptr<GameEntity>> SelectedEntity{nullptr};
    ObservableProperty<std::vector<std::shared_ptr<GameEntity>>> SelectedEntities;
    ObservableProperty<bool> HasSingleSelection{false};
    ObservableProperty<bool> HasMultipleSelection{false};
    ObservableProperty<size_t> SelectionCount{size_t(0)};

    // Transform Properties (for display and editing)
    ObservableProperty<TransformData> CurrentTransform;
    ObservableProperty<TransformData> AverageTransform; // For multi-selection
    ObservableProperty<bool> HasTransform{false};

    // Script Component Properties
    ObservableProperty<bool> HasScript{false};
    ObservableProperty<std::string> ScriptName{""};
    ObservableProperty<std::vector<std::string>> CommonScripts; // For multi-selection
    ObservableProperty<std::vector<std::string>> AvailableScripts;

    // Geometry Component Properties
    ObservableProperty<bool> HasGeometry{false};
    ObservableProperty<std::string> GeometryName{""};
    ObservableProperty<bool> GeometryVisible{true};
    ObservableProperty<GeometryType> GeometryType{GeometryType::PrimitiveType};

    // Material Component Properties
    ObservableProperty<bool> HasMaterial{false};
    ObservableProperty<MaterialType> MaterialTypeUsed{MaterialType::Lambertian};
    ObservableProperty<glm::vec3> MaterialAlbedo{glm::vec3(1.0f)};
    ObservableProperty<float> MaterialRoughness{0.5f};
    ObservableProperty<glm::vec3> MaterialNormal{glm::vec3(0.0f, 0.0f, 1.0f)};
    ObservableProperty<float> MaterialAO{1.0f};
    ObservableProperty<glm::vec3> MaterialEmissive{glm::vec3(0.0f)};
    ObservableProperty<float> MaterialIOR{1.5f};
    ObservableProperty<float> MaterialTransparency{0.0f};
    ObservableProperty<float> MaterialMetallic{0.0f};

    // Physics Component Properties
    ObservableProperty<bool> HasPhysics{false};
    ObservableProperty<float> Mass;
    ObservableProperty<bool> IsKinematic;
    ObservableProperty<glm::vec3> Inertia;

    // Drone Component Properties
    ObservableProperty<bool> HasDrone{false};
    ObservableProperty<control_abstraction> DroneControlAbstraction{control_abstraction::CMD_MOTOR_SPEEDS};
    ObservableProperty<trajectory_type> DroneTrajectoryType{trajectory_type::Circular};
    ObservableProperty<float> DroneMass{0.5f};
    ObservableProperty<float> DroneArmLength{0.17f};
    ObservableProperty<glm::vec3> DronePosition{0.0f, 0.0f, 0.0f};
    ObservableProperty<glm::vec3> DroneVelocity{0.0f, 0.0f, 0.0f};
    ObservableProperty<glm::vec4> DroneRotorSpeeds{1788.53f, 1788.53f, 1788.53f, 1788.53f};

    // UI State
    ObservableProperty<bool> IsEditingTransform{false};
    ObservableProperty<std::string> StatusMessage{""};

    // Commands
    std::unique_ptr<RelayCommand<TransformData>> UpdateTransformCommand;
    std::unique_ptr<RelayCommand<glm::vec3>> UpdatePositionCommand;
    std::unique_ptr<RelayCommand<glm::vec3>> UpdateRotationCommand;
    std::unique_ptr<RelayCommand<glm::vec3>> UpdateScaleCommand;
    std::unique_ptr<RelayCommand<>> RemoveScriptCommand;
    std::unique_ptr<RelayCommand<std::string>> AddScriptCommand;
    std::unique_ptr<RelayCommand<>> RemoveGeometryCommand;
    std::unique_ptr<RelayCommand<bool>> SetGeometryVisibilityCommand;
    std::unique_ptr<RelayCommand<>> RandomizeGeometryCommand;
    std::unique_ptr<RelayCommand<>> AddPhysicsCommand;
    std::unique_ptr<RelayCommand<>> RemovePhysicsCommand;
    std::unique_ptr<RelayCommand<>> AddDroneCommand;
    std::unique_ptr<RelayCommand<>> RemoveDroneCommand;
    std::unique_ptr<RelayCommand<>> AddMaterialCommand;
    std::unique_ptr<RelayCommand<>> UpdateMaterialCommand;
    std::unique_ptr<RelayCommand<>> RemoveMaterialCommand;
    std::unique_ptr<RelayCommand<control_abstraction>> UpdateDroneControlCommand;
    std::unique_ptr<RelayCommand<trajectory_type>> UpdateDroneTrajectoryCommand;
    std::unique_ptr<RelayCommand<>> RefreshCommand;

    ComponentViewModel()
    : m_transformService(TransformService::Get())
    {
        InitializeCommands();
        SubscribeToSelectionService();
        SubscribeToEvents();
        LoadAvailableScripts();
    }

    void SetProject(std::shared_ptr<Project> project)
    {
        if (m_project != project) {
            m_project = project;
            LoadAvailableScripts();
            RefreshSelection();
        }
    }

    void StartTransformEdit()
    {
        IsEditingTransform = true;
        m_transformBeforeEdit = CurrentTransform.Get();
    }

    void EndTransformEdit()
    {
        IsEditingTransform = false;
        if (m_transformBeforeEdit != CurrentTransform.Get())
        {
            CreateTransformUndoAction(m_transformBeforeEdit, CurrentTransform.Get());
        }
    }

private:
    std::shared_ptr<Project> m_project;
    TransformService& m_transformService;
    TransformData m_transformBeforeEdit;

    void InitializeCommands() {
        // Update full transform
        UpdateTransformCommand = std::make_unique<RelayCommand<TransformData>>(
            [this](const TransformData& data) { ExecuteUpdateTransform(data); },
            [this](const TransformData&) { return HasSingleSelection.Get(); }
        );

        // Update individual components
        UpdatePositionCommand = std::make_unique<RelayCommand<glm::vec3>>(
            [this](const glm::vec3& pos) {
                auto transform = CurrentTransform.Get();
                transform.position = pos;
                ExecuteUpdateTransform(transform);
            },
            [this](const glm::vec3&) { return HasSingleSelection.Get() || HasMultipleSelection.Get(); }
        );

        UpdateRotationCommand = std::make_unique<RelayCommand<glm::vec3>>(
            [this](const glm::vec3& rot) {
                auto transform = CurrentTransform.Get();
                transform.rotation = rot;
                ExecuteUpdateTransform(transform);
            },
            [this](const glm::vec3&) { return HasSingleSelection.Get() || HasMultipleSelection.Get(); }
        );

        UpdateScaleCommand = std::make_unique<RelayCommand<glm::vec3>>(
            [this](const glm::vec3& scale) {
                auto transform = CurrentTransform.Get();
                transform.scale = scale;
                ExecuteUpdateTransform(transform);
            },
            [this](const glm::vec3&) { return HasSingleSelection.Get() || HasMultipleSelection.Get(); }
        );

        // Script commands
        AddScriptCommand = std::make_unique<RelayCommand<std::string>>(
            [this](const std::string& scriptName) { ExecuteAddScript(scriptName); },
            [this](const std::string&) { return HasSingleSelection.Get() && !HasScript.Get(); }
        );

        RemoveScriptCommand = std::make_unique<RelayCommand<>>(
            [this]() { ExecuteRemoveScript(); },
            [this]() { return HasScript.Get(); }
        );

        // Geometry commands
        SetGeometryVisibilityCommand = std::make_unique<RelayCommand<bool>>(
            [this](bool visible) { ExecuteSetGeometryVisibility(visible); },
            [this](bool) { return HasGeometry.Get(); }
        );

        RandomizeGeometryCommand = std::make_unique<RelayCommand<>>(
            [this]() { ExecuteRandomizeGeometry(); },
            [this]() { return HasGeometry.Get() && HasSingleSelection.Get(); }
        );

        // Physics commands
        AddPhysicsCommand = std::make_unique<RelayCommand<>>(
            [this]() { ExecuteAddPhysics(); },
            [this]() { return HasSingleSelection.Get() && HasGeometry.Get() && !HasPhysics.Get(); }
        );

        RemovePhysicsCommand= std::make_unique<RelayCommand<>>(
            [this]() {},
            [this]() { return HasPhysics.Get(); }
        );

        // Material Commands
        AddMaterialCommand = std::make_unique<RelayCommand<>>(
            [this]() { ExecuteAddMaterial(); },
            [this]() { return HasSingleSelection.Get() && HasGeometry.Get() && !HasMaterial.Get(); }
        );

        RemoveMaterialCommand = std::make_unique<RelayCommand<>>(
            [this]() { ExecuteRemoveMaterial(); },
            [this]() { return HasMaterial.Get(); }
        );

        // Drone Commands
        AddDroneCommand = std::make_unique<RelayCommand<>>(
        [this]() { ExecuteAddDrone(); },
        [this]() { return HasSingleSelection.Get() && HasPhysics.Get() && !HasDrone.Get(); }
    );

        RemoveDroneCommand = std::make_unique<RelayCommand<>>(
            [this]() { ExecuteRemoveDrone(); },
            [this]() { return HasDrone.Get(); }
        );

        UpdateDroneControlCommand = std::make_unique<RelayCommand<control_abstraction>>(
            [this](control_abstraction ca) { ExecuteUpdateDroneControl(ca); },
            [this](control_abstraction) { return HasDrone.Get(); }
        );

        UpdateDroneTrajectoryCommand = std::make_unique<RelayCommand<trajectory_type>>(
            [this](trajectory_type type) { ExecuteUpdateDroneTrajectory(type); },
            [this](trajectory_type) { return HasDrone.Get(); }
        );

        UpdateMaterialCommand = std::make_unique<RelayCommand<>>(
            [this]() { ExecuteUpdateMaterial(); },
            [this]() { return HasMaterial.Get(); }
        );

        // Refresh command
        RefreshCommand = std::make_unique<RelayCommand<>>(
            [this]() { RefreshSelection(); }
        );
    }

    void SubscribeToSelectionService() {
        auto& selectionService = SelectionService::Get();

        selectionService.SubscribeToMultiSelectionChange(
            [this](const std::unordered_set<uint32_t>& selectedIds) {
                HandleSelectionChanged(selectedIds);
            }
        );
    }

    void SubscribeToEvents() {
        EventBus::Get().Subscribe<EntityRemovedEvent>(
            [this](const EntityRemovedEvent& e) {
                RefreshSelection();
            }
        );

        EventBus::Get().Subscribe<SceneChangedEvent>(
            [this](const SceneChangedEvent& e) {
                RefreshSelection();
            }
        );

        EventBus::Get().Subscribe<EntityMovedEvent>(
            [this](const EntityMovedEvent& e) {
               RefreshSelection();
            }
        );
    }

    void HandleSelectionChanged(const std::unordered_set<uint32_t>& selectedIds) {
        if (!m_project || !m_project->GetActiveScene()) {
            ClearSelection();
            return;
        }

        auto scene = m_project->GetActiveScene();
        std::vector<std::shared_ptr<GameEntity>> entities;

        for (auto id : selectedIds) {
            if (auto entity = scene->GetEntity(id)) {
                entities.push_back(entity);
            }
        }

        SelectedEntities = entities;
        SelectionCount = entities.size();
        HasSingleSelection = (entities.size() == 1);
        HasMultipleSelection = (entities.size() > 1);

        if (HasSingleSelection.Get()) {
            SelectedEntity = entities[0];
            RefreshSingleSelection(entities[0]);
        } else if (HasMultipleSelection.Get()) {
            SelectedEntity = std::shared_ptr<GameEntity>(nullptr);
            RefreshMultiSelection(entities);
        } else {
            ClearSelection();
        }
    }

    void RefreshSingleSelection(std::shared_ptr<GameEntity> entity) {
        if (!entity) return;

        // Transform
        if (auto* transform = entity->GetComponent<Transform>()) {
            HasTransform = true;
            CurrentTransform = m_transformService.GetEntityTransform(entity->GetID());
        } else {
            HasTransform = false;
        }

        // Script
        if (auto* script = entity->GetComponent<Script>()) {
            HasScript = true;
            ScriptName = script->GetScriptName();
        } else {
            HasScript = false;
            ScriptName = std::string("");
        }

        // Geometry
        if (auto* geometry = entity->GetComponent<Geometry>()) {
            HasGeometry = true;
            GeometryName = geometry->GetGeometryName();
            GeometryVisible = geometry->IsVisible();
            GeometryType = geometry->GetGeometryType();
        } else {
            HasGeometry = false;
            GeometryName = std::string("");
        }

        if (auto* physic = entity->GetComponent<Physics>())
        {
            HasPhysics = true;
            Mass = physic->GetMass();
            Inertia = physic->GetInertia();
            IsKinematic = physic->IsKinematic();
        }
        else
        {
            HasPhysics = false;
        }

        // Material
        if (auto* material = entity->GetComponent<Material>())
        {
            RefreshMaterialComponent(entity);
        }
        else
        {
            HasMaterial = false;
        }

        if (entity->GetComponent<Drone>())
        {
            RefreshDroneComponent(entity);
        }
    }

    void RefreshMultiSelection(const std::vector<std::shared_ptr<GameEntity>>& entities) {
        if (entities.empty()) return;

        // Calculate average transform
        TransformData avgTransform;
        int transformCount = 0;

        for (const auto& entity : entities) {
            if (entity->GetComponent<Transform>()) {
                auto data = m_transformService.GetEntityTransform(entity->GetID());
                avgTransform.position += data.position;
                avgTransform.rotation += data.rotation;
                avgTransform.scale += data.scale;
                transformCount++;
            }
        }

        if (transformCount > 0) {
            avgTransform.position /= static_cast<float>(transformCount);
            avgTransform.rotation /= static_cast<float>(transformCount);
            avgTransform.scale /= static_cast<float>(transformCount);
            HasTransform = true;
        } else {
            HasTransform = false;
        }

        AverageTransform = avgTransform;
        CurrentTransform = avgTransform;

        // Find common scripts
        std::vector<std::string> commonScripts;
        bool firstEntity = true;

        for (const auto& entity : entities) {
            if (auto* script = entity->GetComponent<Script>()) {
                if (firstEntity) {
                    commonScripts.push_back(script->GetScriptName());
                    firstEntity = false;
                } else {
                    // Check if this script is common to all
                    auto it = std::find(commonScripts.begin(), commonScripts.end(),
                                      script->GetScriptName());
                    if (it == commonScripts.end()) {
                        commonScripts.clear();
                        break;
                    }
                }
            }
        }

        CommonScripts = commonScripts;
        HasScript = !commonScripts.empty();

        // Geometry is not shown for multi-selection
        HasGeometry = false;
    }

    void ClearSelection() {
        SelectedEntity = std::shared_ptr<GameEntity>(nullptr);
        SelectedEntities.Set({});
        HasSingleSelection = false;
        HasMultipleSelection = false;
        SelectionCount = 0;
        HasTransform = false;
        HasScript = false;
        HasGeometry = false;
        HasPhysics = false;
        HasMaterial = false;
    }

    void RefreshSelection() {
        auto& selectionService = SelectionService::Get();
        HandleSelectionChanged(selectionService.GetSelectedEntities());
    }

    void RefreshMaterialComponent(std::shared_ptr<GameEntity> entity) {
        if (auto* material = entity->GetComponent<Material>()) {
            HasMaterial = true;
            MaterialTypeUsed = material->GetMaterialType();
            MaterialAlbedo = material->GetAlbedo();
            MaterialRoughness = material->GetRoughness();
            MaterialNormal = material->GetNormal();
            MaterialAO = material->GetAO();
            MaterialEmissive = material->GetEmissive();
            MaterialIOR = material->GetIOR();
            MaterialTransparency = material->GetTransparency();
            MaterialMetallic = material->GetMetallic();
        } else {
            HasMaterial = false;
            MaterialTypeUsed = MaterialType::Lambertian;
            MaterialAlbedo = glm::vec3(1.0f);
            MaterialRoughness = 0.5f;
            MaterialNormal = glm::vec3(0.0f, 0.0f, 1.0f);
            MaterialAO = 1.0f;
            MaterialEmissive = glm::vec3(0.0f);
            MaterialIOR = 1.5f;
            MaterialTransparency = 0.0f;
            MaterialMetallic = 0.0f;
        }
    }

    void RefreshDroneComponent(std::shared_ptr<GameEntity> entity) {
        if (auto* drone = entity->GetComponent<Drone>()) {
            HasDrone = true;
            DroneControlAbstraction = drone->GetControlAbstraction();

            const auto& params = drone->GetParams();
            DroneMass = params.i.mass;
            DroneArmLength = glm::length(params.g.rotor_positions[0]);

            const auto& trajectory = drone->GetTrajectory();
            DroneTrajectoryType = trajectory.type;

            const auto& state = drone->GetDroneState();
            DronePosition = state.position;
            DroneVelocity = state.velocity;
            DroneRotorSpeeds = state.rotor_speeds;
        } else {
            HasDrone = false;
            // Reset to defaults
            DroneControlAbstraction = control_abstraction::CMD_MOTOR_SPEEDS;
            DroneTrajectoryType = trajectory_type::Circular;
            DroneMass = 1.0f;
            DroneArmLength = 0.25f;
            DronePosition = glm::vec3(0.0f);
            DroneVelocity = glm::vec3(0.0f);
            DroneRotorSpeeds = glm::vec4(0.0f);
        }
    }

    void ExecuteAddMaterial() {
        if (!SelectedEntity.Get() || !HasGeometry.Get()) return;

        MaterialInitializer materialInit;
        materialInit.material = PBRMaterial{
            MaterialType::Lambertian,
            glm::vec3(1.0f, 0.0f ,0.0f),
            0.0f,
            glm::vec3(0.0f, 0.0f, 1.0f),
            1.0f,
            glm::vec3(0.0f),
            1.5f,
            0.0f,
            0.0f
        };

        if (auto* material = SelectedEntity.Get()->AddComponent<Material>(&materialInit)) {
            HasMaterial = true;
            RefreshMaterialComponent(SelectedEntity.Get());

            if (m_project && m_project->GetActiveScene()) {
                m_project->GetActiveScene()->UpdateEntity(SelectedEntity.Get()->GetID());
            }

            UpdateStatus("Material component added");
            Logger::Get().Log(MessageType::Info, "Added material component");
        }
    }

    void ExecuteUpdateMaterial()
    {
        if (!SelectedEntity.Get() || !HasMaterial.Get()) return;

        if (auto* material = SelectedEntity.Get()->GetComponent<Material>())
        {
            material->SetMaterialType(MaterialTypeUsed.Get());
            material->SetAlbedo(MaterialAlbedo.Get());
            material->SetRoughness(MaterialRoughness.Get());
            material->SetNormal(MaterialNormal.Get());
            material->SetAO(MaterialAO.Get());
            material->SetEmissive(MaterialEmissive.Get());
            material->SetIOR(MaterialIOR.Get());
            material->SetTransparency(MaterialTransparency.Get());
            material->SetMetallic(MaterialMetallic.Get());


            if (m_project && m_project->GetActiveScene()) {
                m_project->GetActiveScene()->UpdateEntity(SelectedEntity.Get()->GetID());
            }

            RefreshMaterialComponent(SelectedEntity.Get());

            MaterialUpdatedEvent event;
            event.entityId = SelectedEntity.Get()->GetID();
            EventBus::Get().Publish(event);

            UpdateStatus("Material updated");
        }
    }

    void ExecuteRemoveMaterial() {
        if (!SelectedEntity.Get()) return;

        if (SelectedEntity.Get()->RemoveComponent<Material>()) {
            HasMaterial = false;

            if (m_project && m_project->GetActiveScene()) {
                m_project->GetActiveScene()->UpdateEntity(SelectedEntity.Get()->GetID());
            }

            UpdateStatus("Material component removed");
            Logger::Get().Log(MessageType::Info, "Removed material component");
        }
    }

    void ExecuteAddDrone() {
        if (!SelectedEntity.Get() || !HasPhysics.Get()) return;

        DroneInitializer droneInit;
        droneInit.params = CreateDefaultQuadParams();
        droneInit.control_abstraction = control_abstraction::CMD_VEL;
        droneInit.trajectory = CreateDefaultTrajectory();
        droneInit.drone_state = CreateDefaultDroneState();
        droneInit.input = control_input{};

        if (auto* drone = SelectedEntity.Get()->AddComponent<Drone>(&droneInit)) {
            HasDrone = true;
            RefreshDroneComponent(SelectedEntity.Get());

            // Update entity in scene
            if (m_project && m_project->GetActiveScene()) {
                m_project->GetActiveScene()->UpdateEntity(SelectedEntity.Get()->GetID());
            }

            UpdateStatus("Drone component added");
            Logger::Get().Log(MessageType::Info, "Added drone component");
        }
    }

    void ExecuteRemoveDrone() {
        if (!SelectedEntity.Get()) return;

        if (SelectedEntity.Get()->RemoveComponent<Drone>()) {
            HasDrone = false;

            // Update entity in scene
            if (m_project && m_project->GetActiveScene()) {
                m_project->GetActiveScene()->UpdateEntity(SelectedEntity.Get()->GetID());
            }

            UpdateStatus("Drone component removed");
            Logger::Get().Log(MessageType::Info, "Removed drone component");
        }
    }

    void ExecuteUpdateDroneControl(control_abstraction ca) {
        if (!SelectedEntity.Get() || !HasDrone.Get()) return;

        if (auto* drone = SelectedEntity.Get()->GetComponent<Drone>()) {
            drone->SetControlAbstraction(ca);
            DroneControlAbstraction = ca;

            // Update entity in scene
            if (m_project && m_project->GetActiveScene()) {
                m_project->GetActiveScene()->UpdateEntity(SelectedEntity.Get()->GetID());
            }

            UpdateStatus("Drone control mode updated");
        }
    }

    void ExecuteUpdateDroneTrajectory(trajectory_type type) {
        if (!SelectedEntity.Get() || !HasDrone.Get()) return;

        if (auto* drone = SelectedEntity.Get()->GetComponent<Drone>()) {
            auto& traj = drone->GetTrajectory();
            traj.type = type;
            DroneTrajectoryType = type;

            // Update entity in scene
            if (m_project && m_project->GetActiveScene()) {
                m_project->GetActiveScene()->UpdateEntity(SelectedEntity.Get()->GetID());
            }

            UpdateStatus("Drone trajectory updated");
        }
    }

    [[nodiscard]] quad_params CreateDefaultQuadParams() const {
        quad_params params;

        // Inertia properties
        params.i.mass = DroneMass.Get();
        params.i.principal_inertia = {3.65e-3f, 3.68e-3f, 7.03e-3f};
        params.i.product_inertia = {0.0f, 0.0f, 0.0f};

        // Geometry properties
        float arm = DroneArmLength.Get();
        const float sqrt2_2 = 0.70710678118f;
        params.g.rotor_radius = 0.1f;
        params.g.rotor_positions[0] = {arm * sqrt2_2, arm * sqrt2_2, 0.0f}; // Front right
        params.g.rotor_positions[1] = {arm * sqrt2_2, -arm * sqrt2_2, 0.0f}; // back right
        params.g.rotor_positions[2] = {-arm * sqrt2_2, -arm * sqrt2_2, 0.0f}; // back left
        params.g.rotor_positions[3] = {-arm * sqrt2_2, arm * sqrt2_2, 0.0f}; // front left
        params.g.rotor_directions = glm::vec4(1, -1, 1, -1);

        // Aerodynamics
        params.a.parasitic_drag = {0.5e-2f, 0.5e-2f, 1e-2f};

        // Rotor properties
        params.r.k_eta = 5.57e-06f;
        params.r.k_m = 1.36e-07f;
        params.r.k_d = 1.19e-04f;
        params.r.k_z = 2.32e-04f;
        params.r.k_h = 3.39e-3f;
        params.r.k_flap = 0.0f;

        // Motor properties
        params.m.tau_m = 0.005f;
        params.m.rotor_speed_min = 0.0f;
        params.m.rotor_speed_max = 1500.0f;
        params.m.motor_noise_std = 0.0f;

        // Control gains
        params.c.kp_pos = glm::vec3(6.5f, 6.5f, 15.0f);
        params.c.kd_pos = glm::vec3(4.0f, 4.0f, 9.0f);
        params.c.kp_att = 544.0f;
        params.c.kd_att = 46.64f;
        params.c.kp_vel = glm::vec3(0.65f, 0.65f, 1.5f);

        // Lower level controller
        params.l.k_w = 1;
        params.l.k_v = 10;
        params.l.kp_att = 544;
        params.l.kd_att = 46.64f;

        return params;
    }

    trajectory CreateDefaultTrajectory() const {
        trajectory traj;
        traj.type = DroneTrajectoryType.Get();
        traj.position = glm::vec3(0.0f);
        traj.radius = 1.0f;
        traj.frequency = 0.5f;
        traj.delta = 1.0f;
        traj.n_points = 10;
        traj.segment_time = 1.0f;
        return traj;
    }

    drone_state CreateDefaultDroneState() const {
        drone_state state;
        state.position = DronePosition.Get();
        state.velocity = DroneVelocity.Get();
        state.attitude = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        state.body_rates = glm::vec3(0.0f);
        state.wind = glm::vec3(0.0f);
        state.rotor_speeds = DroneRotorSpeeds.Get();
        return state;
    }

    void ExecuteUpdateTransform(const TransformData& data) {
        if (HasSingleSelection.Get() && SelectedEntity.Get()) {
            m_transformService.UpdateEntityTransform(SelectedEntity.Get(), data);
            CurrentTransform = data;
            UpdateStatus("Transform updated");
        } else if (HasMultipleSelection.Get()) {
            // For multi-selection, apply relative changes
            TransformData avgBefore = AverageTransform.Get();
            glm::vec3 deltaPos = data.position - avgBefore.position;
            glm::vec3 deltaRot = data.rotation - avgBefore.rotation;
            glm::vec3 deltaScale = data.scale - avgBefore.scale;

            m_transformService.BatchUpdateTransforms(
                SelectedEntities.Get(),
                [&](const TransformData& current) {
                    TransformData newData = current;
                    newData.position += deltaPos;
                    newData.rotation += deltaRot;
                    newData.scale += deltaScale;
                    return newData;
                }
            );

            AverageTransform = data;
            CurrentTransform = data;
            UpdateStatus("Batch transform updated");
        }
    }

    void ExecuteAddScript(const std::string& scriptName)
    {
        if (!SelectedEntity.Get()) return;

        ScriptInitializer scriptInit;
        scriptInit.scriptName = scriptName;

        if (auto* script = SelectedEntity.Get()->AddComponent<Script>(&scriptInit)) {
            HasScript = true;
            ScriptName = scriptName;

            // Update entity in scene
            if (m_project && m_project->GetActiveScene()) {
                m_project->GetActiveScene()->UpdateEntity(SelectedEntity.Get()->GetID());
            }

            UpdateStatus("Script added: " + scriptName);
            Logger::Get().Log(MessageType::Info, "Added script: " + scriptName);
        }
    }

    void ExecuteAddPhysics()
    {
        if (!SelectedEntity.Get() || !SelectedEntity.Get()->GetComponent<Geometry>()) return;

        PhysicInitializer physicInit;
        physicInit.mass = 1.0f;
        physicInit.inertia = glm::vec3(1.0f, 1.0f, 1.0f);
        physicInit.is_kinematic = false;

        if (auto* physic = SelectedEntity.Get()->AddComponent<Physics>(&physicInit))
        {
            // Update the observable properties
            HasPhysics = true;
            Mass = physic->GetMass();
            Inertia = physic->GetInertia();
            IsKinematic = physic->IsKinematic();

            // Update entity in scene
            if (m_project && m_project->GetActiveScene()) {
                m_project->GetActiveScene()->UpdateEntity(SelectedEntity.Get()->GetID());
            }

            UpdateStatus("Physics added");
            Logger::Get().Log(MessageType::Info, "Added Physics");
        }
    }

    void ExecuteRemoveScript()
    {
        if (!SelectedEntity.Get()) return;

        if (SelectedEntity.Get()->RemoveComponent<Script>()) {
            HasScript = false;
            ScriptName = std::string("");

            // Update entity in scene
            if (m_project && m_project->GetActiveScene()) {
                m_project->GetActiveScene()->UpdateEntity(SelectedEntity.Get()->GetID());
            }

            UpdateStatus("Script removed");
            Logger::Get().Log(MessageType::Info, "Removed script");
        }
    }

    void ExecuteSetGeometryVisibility(bool visible) {
        if (!SelectedEntity.Get()) return;

        if (auto* geometry = SelectedEntity.Get()->GetComponent<Geometry>()) {
            geometry->SetVisible(visible);
            GeometryVisible = visible;
            UpdateStatus(visible ? "Geometry shown" : "Geometry hidden");

            GeometryVisibilityChangedEvent event;
            event.entityId = SelectedEntity.Get()->GetID();
            event.visible = visible;
            EventBus::Get().Publish(event);
        }
    }

    void ExecuteRandomizeGeometry() {
        if (!SelectedEntity.Get()) return;

        auto* geometry = SelectedEntity.Get()->GetComponent<Geometry>();
        if (!geometry || !geometry->GetScene()) return;

        auto& mesh = geometry->GetScene()->lod_groups[0].meshes[0];

        lark::editor::Geometry::randomModificationVertexes(
            SelectedEntity.Get()->GetID(),
            mesh.vertices.size(),
            mesh.positions
        );

        UpdateStatus("Geometry randomized");
        Logger::Get().Log(MessageType::Info, "Randomized geometry vertices");
    }

    void LoadAvailableScripts() {
        if (!m_project) {
            AvailableScripts.Set({});
            return;
        }

        size_t scriptCount = 0;
        const char** scriptNames = GetScriptNames(&scriptCount);

        std::vector<std::string> scripts;
        for (size_t i = 0; i < scriptCount; ++i) {
            scripts.push_back(scriptNames[i]);
        }

        AvailableScripts = scripts;
    }

    void CreateTransformUndoAction(const TransformData& oldData, const TransformData& newData) {
        if (!m_project || !SelectedEntity.Get()) return;

        auto entity = SelectedEntity.Get();
        uint32_t entityId = entity->GetID();

        auto action = std::make_shared<UndoRedoAction>(
            [this, entityId, oldData]() {
                if (auto scene = m_project->GetActiveScene()) {
                    if (auto entity = scene->GetEntity(entityId)) {
                        m_transformService.UpdateEntityTransform(entity, oldData);
                        if (SelectedEntity.Get() && SelectedEntity.Get()->GetID() == entityId) {
                            CurrentTransform = oldData;
                        }
                    }
                }
            },
            [this, entityId, newData]() {
                if (auto scene = m_project->GetActiveScene()) {
                    if (auto entity = scene->GetEntity(entityId)) {
                        m_transformService.UpdateEntityTransform(entity, newData);
                        if (SelectedEntity.Get() && SelectedEntity.Get()->GetID() == entityId) {
                            CurrentTransform = newData;
                        }
                    }
                }
            },
            "Transform Change"
        );

        GlobalUndoRedo::Instance().GetUndoRedo().Add(action);
    }

    void UpdateStatus(const std::string& message) {
        StatusMessage = message;
    }
};

