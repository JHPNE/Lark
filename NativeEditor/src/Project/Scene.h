#pragma once
#include <string>
#include <memory>
#include <vector>
#include "GameEntity.h"
#include "../Utils/Logger.h"
#include "../Utils/UndoRedo.h"

// Forward declare Project to avoid circular dependency
class Project;

class Scene : public std::enable_shared_from_this<Scene> {
public:
    Scene(const std::string& name, uint32_t id, std::shared_ptr<Project> owner)
        : m_name(name)
        , m_id(id)
        , m_owner(owner) {}

    const std::string& GetName() const { return m_name; }
    uint32_t GetID() const { return m_id; }
    std::shared_ptr<Project> GetOwner() const { return m_owner; }

    // Entity Management
    std::shared_ptr<GameEntity> CreateEntity(const std::string& name) {
        uint32_t entityId = GenerateEntityID();
        auto entity = std::shared_ptr<GameEntity>(
            new GameEntity(name, entityId, shared_from_this())
        );
        m_entities.push_back(entity);

        Logger::Get().Log(MessageType::Info, "Created entity: " + name);
        return entity;
    }

    bool RemoveEntity(uint32_t entityId) {
        auto it = std::find_if(m_entities.begin(), m_entities.end(),
            [entityId](const auto& entity) { return entity->GetID() == entityId; });

        if (it != m_entities.end()) {
            std::string removedName = (*it)->GetName();
            m_entities.erase(it);
            Logger::Get().Log(MessageType::Info, "Removed entity: " + removedName);
            return true;
        }
        Logger::Get().Log(MessageType::Warning, "Failed to remove entity with ID: " + std::to_string(entityId));
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

private:
    uint32_t GenerateEntityID() const {
        return m_entities.empty() ? 1 :
            std::max_element(m_entities.begin(), m_entities.end(),
                [](const auto& a, const auto& b) { return a->GetID() < b->GetID(); }
            )->get()->GetID() + 1;
    }

    std::string m_name;
    uint32_t m_id;
    std::shared_ptr<Project> m_owner;
    std::vector<std::shared_ptr<GameEntity>> m_entities;
};