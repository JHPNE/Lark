#pragma once
#include <string>
#include <memory>
#include <vector>
#include "GameEntity.h"
#include "../Utils/Etc/Logger.h"
#include "../Utils/System/GlobalUndoRedo.h"
#include "EngineAPI.h"
#include "Utils/System/Serialization.h"

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
            uint32_t entityId = entity->GetID();
            auto action = std::make_shared<UndoRedoAction>(
                [this, entityId]() {
                    RemoveEntityInternal(entityId);
                },
                [this, name]() {
                    CreateEntityInternal(name);
                },
                "Add Scene: " + name
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

        // Store entity data for restoration
        std::string entityName = entity->GetName();
        bool wasActive = entity->IsActive();

        if (RemoveEntityInternal(entityId)) {
            auto action = std::make_shared<UndoRedoAction>(
                // Undo function - recreates the entity with original data
                [this, entityName, wasActive]() {
                    auto restoredEntity = CreateEntityInternal(entityName);
                    if (restoredEntity) {
                        restoredEntity->SetActive(wasActive);
                    }
                },
                // Redo function - removes the entity again
                [this, entityId]() {
                    RemoveEntityInternal(entityId);
                },
                "Remove Entity: " + entityName
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
};