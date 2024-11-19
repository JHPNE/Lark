#pragma once
#include <memory>
#include <vector>
#include <unordered_map>
#include "../Components/Component.h"
#include "../Components/Transform.h"
#include "../Components/Script.h"

class Scene;

class GameEntity {
public:
    template<typename T>
    T* GetComponent() const {
        static_assert(std::is_base_of<Component, T>::value, "T must derive from Component");

        ComponentType type = T::GetStaticType(); // Temporary to get type
        auto it = m_components.find(type);
        return it != m_components.end() ? static_cast<T*>(it->second.get()) : nullptr;
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
        , m_isEnabled(true) {}

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