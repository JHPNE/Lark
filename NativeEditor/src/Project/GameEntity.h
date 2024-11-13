#pragma once
#include <string>
#include <memory>

class Scene;

class GameEntity {
public:
    // Allow Scene to create/destroy entities
    friend class Scene;

    // Get basic properties
    const std::string& GetName() const { return m_name; }
    uint32_t GetID() const { return m_id; }
    bool IsEnabled() const { return m_isEnabled; }
    std::shared_ptr<Scene> GetScene() const { return m_scene; }

    // Set basic properties
    void SetName(const std::string& name) { m_name = name; }
    void SetEnabled(bool enabled) { m_isEnabled = enabled; }

private:
    // Private constructor - only Scene can create entities
    GameEntity(const std::string& name, uint32_t id, std::shared_ptr<Scene> scene)
        : m_name(name)
        , m_id(id)
        , m_scene(scene)
        , m_isEnabled(true) {}

    std::string m_name;
    uint32_t m_id;
    bool m_isEnabled;
    std::shared_ptr<Scene> m_scene;
};