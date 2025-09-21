#pragma once
#include "../Utils/Etc/Logger.h"
#include "../Utils/MathUtils.h"
#include "../Utils/System/GlobalUndoRedo.h"
#include "Components/Geometry.h"
#include "EngineAPI.h"
#include "GameEntity.h"
#include "Components/Drone.h"
#include "Components/Physics.h"
#include "Utils/Utils.h"
#include <memory>
#include <string>
#include <vector>

using namespace MathUtils;

// Forward declare Project to avoid circular dependency
class Project;

class Scene : public std::enable_shared_from_this<Scene>
{
  public:
    Scene(const std::string &name, uint32_t id, std::shared_ptr<Project> owner)
        : m_name(name), m_id(id), m_owner(owner), m_isActive(false)
    {
    }

    const std::string &GetName() const { return m_name; }
    uint32_t GetID() const { return m_id; }
    std::shared_ptr<Project> GetOwner() const { return m_owner; }

    // Entity Management
    std::shared_ptr<GameEntity> CreateEntity(const std::string &name)
    {
        auto entity = CreateEntityInternal(name);

        if (entity)
        {
            // Store the newly created entity's ID
            uint32_t entityId = entity->GetID();

            // Create a capture for the current entity state
            EntityState entityState{.name = name, .id = entityId, .isActive = entity->IsActive()};

            auto action = std::make_shared<UndoRedoAction>(
                // Undo function - uses the stored state
                [this, entityState]()
                {
                    // Find entity by both ID and name for safety
                    auto entity = std::find_if(m_entities.begin(), m_entities.end(),
                                               [&entityState](const auto &e)
                                               {
                                                   return e->GetID() == entityState.id ||
                                                          e->GetName() == entityState.name;
                                               });

                    if (entity != m_entities.end())
                    {
                        RemoveEntityInternal((*entity)->GetID());
                    }
                },
                // Redo function - recreates with stored state
                [this, entityState]() -> std::shared_ptr<GameEntity>
                {
                    auto newEntity = CreateEntityInternal(entityState.name);
                    if (newEntity)
                    {
                        newEntity->SetActive(entityState.isActive);
                    }
                    return newEntity;
                },
                "Add Entity: " + name);

            GlobalUndoRedo::Instance().GetUndoRedo().Add(action);
        }

        return entity;
    }

    std::shared_ptr<GameEntity> CreateEntityInternal(const std::string &name)
    {
        std::shared_ptr<GameEntity> entity;

        // Create basic engine entity with just transform
        game_entity_descriptor desc{};
        desc.transform.position[0] = desc.transform.position[1] = desc.transform.position[2] = 0.f;
        std::memset(desc.transform.rotation, 0, sizeof(float) * 3);
        desc.transform.scale[0] = desc.transform.scale[1] = desc.transform.scale[2] = 1.0f;

        uint32_t entityId = CreateGameEntity(&desc);
        entity = std::shared_ptr<GameEntity>(new GameEntity(name, entityId, shared_from_this()));

        entity->SetActive(m_isActive);
        m_entities.push_back(entity);
        Logger::Get().Log(MessageType::Info, "Created entity: " + name);
        return entity;
    }

    bool RemoveEntityInternal(uint32_t entityId)
    {
        auto it = std::find_if(m_entities.begin(), m_entities.end(), [entityId](const auto &entity)
                               { return entity->GetID() == entityId; });

        if (it != m_entities.end())
        {

            // Uses EngineDLL
            RemoveGameEntity(entityId);

            std::string removedName = (*it)->GetName();
            m_entities.erase(it);
            Logger::Get().Log(MessageType::Info, "Removed entity: " + removedName);
            return true;
        }
        Logger::Get().Log(MessageType::Warning,
                          "Failed to remove entity with ID: " + std::to_string(entityId));
        return false;
    }

    bool RemoveEntity(uint32_t entityId)
    {
        // Get the entity before removal to store its data for undo
        auto entity = GetEntity(entityId);
        if (!entity)
        {
            Logger::Get().Log(MessageType::Warning,
                              "Cannot remove entity - ID not found: " + std::to_string(entityId));
            return false;
        }

        // Store complete entity state for restoration
        EntityState entityState{
            .name = entity->GetName(), .id = entity->GetID(), .isActive = entity->IsActive()};

        if (RemoveEntityInternal(entityId))
        {
            auto action = std::make_shared<UndoRedoAction>(
                // Undo function - recreates with stored state
                [this, entityState]() -> std::shared_ptr<GameEntity>
                {
                    auto restoredEntity = CreateEntityInternal(entityState.name);
                    if (restoredEntity)
                    {
                        restoredEntity->SetActive(entityState.isActive);
                    }
                    return restoredEntity;
                },
                // Redo function - removes by current ID and name
                [this, entityState]()
                {
                    auto entity = std::find_if(m_entities.begin(), m_entities.end(),
                                               [&entityState](const auto &e)
                                               {
                                                   return e->GetID() == entityState.id ||
                                                          e->GetName() == entityState.name;
                                               });

                    if (entity != m_entities.end())
                    {
                        RemoveEntityInternal((*entity)->GetID());
                    }
                },
                "Remove Entity: " + entityState.name);

            GlobalUndoRedo::Instance().GetUndoRedo().Add(action);
            return true;
        }

        return false;
    }

    void RemoveAllEntities() { m_entities.clear(); }

    void UpdateEntity(uint32_t entityId)
    {
        auto entity = GetEntity(entityId);
        game_entity_descriptor desc{};

        // Fill transform
        if (auto *transform = entity->GetComponent<Transform>())
        {
            const auto &pos = transform->GetPosition();
            const auto &rot = transform->GetRotation();
            const auto &scale = transform->GetScale();
            Utils::SetTransform(desc, pos, rot, scale);
        }

        // Fill script
        if (auto *script = entity->GetComponent<Script>())
        {
            desc.script.script_creator = GetScriptCreator(script->GetScriptName().c_str());
        }

        // Fill geometry
        if (auto *geometry = entity->GetComponent<Geometry>())
        {
            // Only set geometry descriptor if we have a valid scene
            if (geometry->GetScene())
            {
                desc.geometry.is_dynamic = false;
                desc.geometry.scene = geometry->GetScene();

                // Fill physic
                if (auto *physic = entity->GetComponent<Physics>())
                {
                    auto *transform = entity->GetComponent<Transform>();
                    desc.physics.scene = geometry->GetScene();
                    desc.physics.is_kinematic = physic->IsKinematic();
                    desc.physics.position = transform->GetPosition();

                    // Calculate orientation from transform
                    glm::quat orientation = glm::quat(glm::radians(transform->GetRotation()));
                    desc.physics.orientation[0] = orientation.w;
                    desc.physics.orientation[1] = orientation.x;
                    desc.physics.orientation[2] = orientation.y;
                    desc.physics.orientation[3] = orientation.z;

                    // Set other physics properties
                    desc.physics.mass = physic->GetMass();
                    const glm::vec3& inertia = physic->GetInertia();
                    desc.physics.inertia[0] = inertia.x;
                    desc.physics.inertia[1] = inertia.y;
                    desc.physics.inertia[2] = inertia.z;


                    if (auto *drone = entity->GetComponent<Drone>())
                    {
                        desc.drone.params = drone->GetParams();
                        desc.drone.control_abstraction = drone->GetControlAbstraction();
                        desc.drone.drone_state = drone->GetDroneState();
                        desc.drone.input = drone->GetControlInput();
                        desc.drone.trajectory = drone->GetTrajectory();
                    }
                }
            }
        }

        UpdateGameEntity(entityId, &desc);
    }

    std::shared_ptr<GameEntity> GetEntity(uint32_t entityId) const
    {
        auto it = std::find_if(m_entities.begin(), m_entities.end(), [entityId](const auto &entity)
                               { return entity->GetID() == entityId; });
        return (it != m_entities.end()) ? *it : nullptr;
    }

    const std::vector<std::shared_ptr<GameEntity>> &GetEntities() const { return m_entities; }

    // Undo/Redo
    UndoRedo &GetUndoRedo() { return m_undoRedo; }

    // Active Cycle
    bool IsActive() const { return m_isActive; }
    void SetActive(bool active)
    {
        if (m_isActive == active)
            return;
        m_isActive = active;

        for (auto &entity : m_entities)
        {
            entity->SetActive(active);
        }
    }

  private:
    bool m_isActive;
    std::string m_name;
    uint32_t m_id;
    std::shared_ptr<Project> m_owner;
    std::vector<std::shared_ptr<GameEntity>> m_entities;
    UndoRedo m_undoRedo;

    // Helper struct to store entity state
    struct EntityState
    {
        std::string name;
        uint32_t id;
        bool isActive;
    };
};