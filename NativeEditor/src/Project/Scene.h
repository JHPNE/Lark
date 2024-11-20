#pragma once
#include <string>
#include <memory>
#include <vector>
#include "GameEntity.h"
#include "../Utils/Etc/Logger.h"
#include "../Utils/MathUtils.h"
#include "../Utils/System/GlobalUndoRedo.h"
#include "EngineAPI.h"
#include "Utils/Utils.h"
#include "Utils/System/Serialization.h"

using namespace MathUtils;

// Forward declare Project to avoid circular dependency
class Project;

class Scene : public std::enable_shared_from_this<Scene> {
public:
    Scene(const std::string& name, uint32_t id, std::shared_ptr<Project> owner)
        : m_name(name)
        , m_id(id)
        , m_owner(owner)
        , m_isActive(false) {}

    const std::string& GetName() const { return m_name; }
    uint32_t GetID() const { return m_id; }
    std::shared_ptr<Project> GetOwner() const { return m_owner; }

    // Entity Management
    std::shared_ptr<GameEntity> CreateEntity(const std::string& name) {
        auto entity = CreateEntityInternal(name);

        if (entity) {
            // Store the newly created entity's ID
            uint32_t entityId = entity->GetID();

            // Create a capture for the current entity state
            EntityState entityState{
                .name = name,
                .id = entityId,
                .isActive = entity->IsActive()
            };

            auto action = std::make_shared<UndoRedoAction>(
                // Undo function - uses the stored state
                [this, entityState]() {
                    // Find entity by both ID and name for safety
                    auto entity = std::find_if(m_entities.begin(), m_entities.end(),
                        [&entityState](const auto& e) {
                            return e->GetID() == entityState.id || e->GetName() == entityState.name;
                        });

                    if (entity != m_entities.end()) {
                        RemoveEntityInternal((*entity)->GetID());
                    }
                },
                // Redo function - recreates with stored state
                [this, entityState]() -> std::shared_ptr<GameEntity> {
                    auto newEntity = CreateEntityInternal(entityState.name);
                    if (newEntity) {
                        newEntity->SetActive(entityState.isActive);
                    }
                    return newEntity;
                },
                "Add Entity: " + name
            );

            GlobalUndoRedo::Instance().GetUndoRedo().Add(action);
        }

        return entity;
    }

    std::shared_ptr<GameEntity> CreateEntityInternal(const std::string& name) {
        game_entity_descriptor desc{};
        desc.transform.position[0] = 1.0f;
        desc.transform.position[1] = 2.0f;
        desc.transform.position[2] = 3.0f;
        // Zero out rotation
        std::memset(desc.transform.rotation, 0, sizeof(float) * 3);
        // Set scale to 1
        desc.transform.scale[0] = desc.transform.scale[1] = desc.transform.scale[2] = 1.0f;

        uint32_t entityId = CreateGameEntity(&desc); //TODO: Use EngineAPI for that
        auto entity = std::shared_ptr<GameEntity>(
            new GameEntity(name, entityId, shared_from_this())
        );

		entity->SetActive(m_isActive);
        m_entities.push_back(entity);
        Logger::Get().Log(MessageType::Info, "Created entity: " + name);
        return entity;
    }

    bool RemoveEntityInternal(uint32_t entityId) {
        auto it = std::find_if(m_entities.begin(), m_entities.end(),
            [entityId](const auto& entity) { return entity->GetID() == entityId; });

        if (it != m_entities.end()) {

            // Uses EngineDLL
            RemoveGameEntity(entityId);

            std::string removedName = (*it)->GetName();
            m_entities.erase(it);
            Logger::Get().Log(MessageType::Info, "Removed entity: " + removedName);
            return true;
        }
        Logger::Get().Log(MessageType::Warning, "Failed to remove entity with ID: " + std::to_string(entityId));
        return false;
    }

    bool RemoveEntity(uint32_t entityId) {
        // Get the entity before removal to store its data for undo
        auto entity = GetEntity(entityId);
        if (!entity) {
            Logger::Get().Log(MessageType::Warning, "Cannot remove entity - ID not found: " + std::to_string(entityId));
            return false;
        }

        // Store complete entity state for restoration
        EntityState entityState{
            .name = entity->GetName(),
            .id = entity->GetID(),
            .isActive = entity->IsActive()
        };

        if (RemoveEntityInternal(entityId)) {
            auto action = std::make_shared<UndoRedoAction>(
                // Undo function - recreates with stored state
                [this, entityState]() -> std::shared_ptr<GameEntity> {
                    auto restoredEntity = CreateEntityInternal(entityState.name);
                    if (restoredEntity) {
                        restoredEntity->SetActive(entityState.isActive);
                    }
                    return restoredEntity;
                },
                // Redo function - removes by current ID and name
                [this, entityState]() {
                    auto entity = std::find_if(m_entities.begin(), m_entities.end(),
                        [&entityState](const auto& e) {
                            return e->GetID() == entityState.id || e->GetName() == entityState.name;
                        });

                    if (entity != m_entities.end()) {
                        RemoveEntityInternal((*entity)->GetID());
                    }
                },
                "Remove Entity: " + entityState.name
            );

            GlobalUndoRedo::Instance().GetUndoRedo().Add(action);
            return true;
        }

        return false;
    }

    void RemoveAllEntities() {
        m_entities.clear();
    }

    std::shared_ptr<GameEntity> GetEntity(uint32_t entityId) const {
        auto it = std::find_if(m_entities.begin(), m_entities.end(),
            [entityId](const auto& entity) { return entity->GetID() == entityId; });
        return (it != m_entities.end()) ? *it : nullptr;
    }

    const std::vector<std::shared_ptr<GameEntity>>& GetEntities() const {
        return m_entities;
    }

    // Component Logic
    template<typename T>
    T* AddComponentToEntity(uint32_t entityId, const ComponentInitializer* initializer = nullptr) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");
        static_assert(!std::is_abstract<T>::value, "Cannot create abstract component type");  // Add this check

        auto entity = GetEntity(entityId);
        if (!entity) {
            Logger::Get().Log(MessageType::Error, "Entity not found: " + std::to_string(entityId));
            return nullptr;
        }

        ComponentType type = T::GetStaticType();

        if (entity->m_components[type]) {
            Logger::Get().Log(MessageType::Warning, "Component already exists on entity: " + entity->GetName());
            return nullptr;
        }

        // Create and initialize the new component
        T* component = new T(entity.get());
        if (initializer && !component->Initialize(initializer)) {
            delete component;
            Logger::Get().Log(MessageType::Error, "Failed to initialize component");
            return nullptr;
        }

        // Store component in entity
        entity->m_components[type] = std::unique_ptr<Component>(component);

        // Create descriptor with current state
        game_entity_descriptor desc{};

        // Fill transform data
        if (auto* transform = entity->GetComponent<Transform>()) {
            const auto& pos = transform->GetPosition();
            const auto& rot = transform->GetRotation();
            const auto& scale = transform->GetScale();
            Utils::SetTransform(desc, pos, rot, scale);
        }

        // Fill script data
        if (auto* script = entity->GetComponent<Script>()) {
            desc.script.script_creator = GetScriptCreator(script->GetScriptName().c_str());
        }

        // Remove old engine entity and create new one
        RemoveGameEntity(entityId);
        uint32_t newId = CreateGameEntity(&desc);

        if (newId == 0) { // Assuming 0 is invalid
            entity->m_components.erase(type);
            Logger::Get().Log(MessageType::Error, "Failed to recreate entity in engine");
            return nullptr;
        }

        // Update entity's ID
        entity->SetID(newId);

        // Add to undo/redo system
        auto action = std::make_shared<UndoRedoAction>(
            [this, entityId, type]() {  // Undo: remove component
                auto entity = GetEntity(entityId);
                if (entity) entity->m_components.erase(type);
            },
            [this, entityId, type, initializer]() {  // Redo: add component
                AddComponentToEntity<T>(entityId, initializer);
            },
            "Add Component to Entity: " + entity->GetName()
        );

        GlobalUndoRedo::Instance().GetUndoRedo().Add(action);

        return component;
    }

    template<typename T>
    bool RemoveComponentFromEntity(uint32_t entityId) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

        auto entity = GetEntity(entityId);
        if (entity) {
            // Get the component type
            ComponentType componentType = T::GetStaticType();

            // Since we cant remove transform or shouldn't
            if (!entity->GetComponent<T>()) {
                Logger::Get().Log(MessageType::Warning, "Entity does not have component of requested type");
                return false;
            }

            // Cannot remove Transform component
            if (componentType == ComponentType::Transform) {
                Logger::Get().Log(MessageType::Warning, "Cannot remove Transform component");
                return false;
            }

            RemoveGameEntity(entityId);
            // Create descriptor with current state
            game_entity_descriptor desc{};
            // Store transform data
            if (auto transform = entity->GetComponent<Transform>()) {
                auto pos = transform->GetPosition();
                auto rot = transform->GetRotation();
                auto scale = transform->GetScale();
                Utils::SetTransform(desc, pos, rot, scale);
            }

            // Handle script component specially
            if (componentType != ComponentType::Script && entity->GetComponent<Script>()) {
                desc.script.script_creator = GetScriptCreator(entity->GetComponent<Script>()->GetScriptName().c_str());
            }

            entity->m_components.erase(componentType);

            auto newEntityId = CreateGameEntity(&desc);
            entity->SetID(newEntityId);

            return true;
        }

        return false;
    }

    // Undo/Redo
    UndoRedo& GetUndoRedo() { return m_undoRedo; }

    // Active Cycle
	bool IsActive() const { return m_isActive; }
    void SetActive(bool active) {
        if (m_isActive == active) return;
        m_isActive = active;

        for (auto& entity : m_entities) {
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
    struct EntityState {
        std::string name;
        uint32_t id;
        bool isActive;
    };
};