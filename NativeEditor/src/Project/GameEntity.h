#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include "../Components/Component.h"
#include "../Components/Transform.h"
#include "../Utils/Logger.h"

class Scene;

class GameEntity {
public:
    // Templates for Components
    template<typename T>
    T* AddComponent() {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

        ComponentType type = T(this).GetType(); // Temporary to get type

        if (m_components[type]) {
            Logger::Get().Log(MessageType::Warning, "Component already exists on entity: " + GetName());
            return nullptr;
        }

        T* component = new T(this);
        m_components[type] = std::unique_ptr<Component>(component);

        return component;
    }

    template<typename T>
    T* GetComponent() const {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

        ComponentType type = T(this).GetType(); // Temporary to get type
        auto it = m_components.find(type);
        return it != m_components.end() ? static_cast<T*>(it->second.get()) : nullptr;
    }

    template<typename T>
    bool RemoveComponent() {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

        ComponentType type = T(this).GetType(); // Temporary to get type

        // Don't allow removing Transform component
        if (type == ComponentType::Transform) {
            Logger::Get().Log(MessageType::Warning, "Cannot remove Transform component");
            return false;
        }

        auto it = m_components.find(type);
        if (it != m_components.end()) {
            m_components.erase(it);
            return true;
        }
        return false;
    }

    // Rest of GameEntity implementation...
    const std::string& GetName() const { return m_name; }
    uint32_t GetID() const { return m_id; }
    bool IsEnabled() const { return m_isEnabled; }
    std::shared_ptr<Scene> GetScene() const { return m_scene; }

private:
    friend class Scene; // Only Scene can create entities

    GameEntity(const std::string& name, uint32_t id, std::shared_ptr<Scene> scene)
        : m_name(name)
        , m_id(id)
        , m_scene(scene)
        , m_isEnabled(true) {
        // Always add Transform component
        AddComponent<Transform>();
    }

    std::string m_name;
    uint32_t m_id;
    bool m_isEnabled;
    std::shared_ptr<Scene> m_scene;
    std::unordered_map<ComponentType, std::unique_ptr<Component>> m_components;
};