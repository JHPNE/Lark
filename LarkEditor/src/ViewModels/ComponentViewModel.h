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
    }

    void HandleSelectionChanged(const std::unordered_set<uint32_t>& selectedIds) {
        if (!m_project || !m_project->GetActiveScene()) {
            ClearSelection();
            return;
        }

        auto scene = m_project->GetActiveScene();
        std::vector<std::shared_ptr<GameEntity>> entities;

        for (uint32_t id : selectedIds) {
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
    }

    void RefreshSelection() {
        auto& selectionService = SelectionService::Get();
        HandleSelectionChanged(selectionService.GetSelectedEntities());
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

    void ExecuteAddScript(const std::string& scriptName) {
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

    void ExecuteRemoveScript() {
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

