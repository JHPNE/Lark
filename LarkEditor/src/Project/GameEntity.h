#pragma once
#include <memory>
#include <unordered_map>
#include "../Components/Component.h"
#include "../Components/Transform.h"
#include "../Components/Script.h"
#include "../Utils/Etc/Logger.h"

class Scene;

class GameEntity {
public:
    // Templates for Components
    template<typename T>
    T* AddComponent(const ComponentInitializer* initializer = nullptr) {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

        ComponentType type = T::GetStaticType();

        if (m_components[type]) {
            Logger::Get().Log(MessageType::Warning, "Component already exists on entity: " + GetName());
            return nullptr;
        }

        T* component = new T(this);
        if (!component->Initialize(initializer)) {
            delete component;
            Logger::Get().Log(MessageType::Error, "Failed to initialize component on entity: " + GetName());
            return nullptr;
        }

        m_components[type] = std::unique_ptr<Component>(component);
        return component;
    }

    template<typename T>
    T* GetComponent() const {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

        ComponentType type = T::GetStaticType(); // Temporary to get type
        auto it = m_components.find(type);
        return it != m_components.end() ? static_cast<T*>(it->second.get()) : nullptr;
    }

    template<typename T>
    bool RemoveComponent() {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

        ComponentType type = T::GetStaticType(); // Temporary to get type

        // Don't allow removing Transform component
        if (type == ComponentType::Transform) {
            Logger::Get().Log(MessageType::Warning, "Cannot remove Transform component");
            return false;
        }

        if (type == ComponentType::Script) {
            Logger::Get().Log(MessageType::Warning, "Cannot remove Transform component");
            return false;
        }

        if (type == ComponentType::Geometry) {
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
    void SetID(uint32_t entityId) { m_id = entityId; }
    bool IsEnabled() const { return m_isEnabled; }
	void SetEnabled(bool enabled) { m_isEnabled = enabled; }
    std::shared_ptr<Scene> GetScene() const { return m_scene; }
	void SetSelected(bool highlight) { m_isSelected = highlight; }
	bool IsSelected() const { return m_isSelected; }

    bool IsActive() const { return m_isActive; }

private:
    friend class Scene; // Only Scene can create entities

    GameEntity(const std::string& name, uint32_t id, std::shared_ptr<Scene> scene)
        : m_name(name)
        , m_id(id)
        , m_scene(scene)
        , m_isEnabled(true) {
        AddComponent<Transform>();
    }

    void SetActive(bool active) {
        if (m_isActive == active) return;

        m_isActive = active;
    }

    std::string m_name;
    bool m_isActive;
    uint32_t m_id;
    bool m_isEnabled;
    bool m_isSelected = false;
    std::shared_ptr<Scene> m_scene;
    std::unordered_map<ComponentType, std::unique_ptr<Component>> m_components;
};