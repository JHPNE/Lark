#pragma once
#include <functional>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <string>
#include <glm/glm.hpp>

// Base event class
struct Event
{
    virtual ~Event() = default;
};

// Specific events
struct EntityCreatedEvent : Event
{
    uint32_t entityId;
    uint32_t sceneId;
    std::string entityName;
};

struct EntityRemovedEvent : Event
{
    uint32_t entityId;
    uint32_t sceneId;
};

struct SceneChangedEvent : Event
{
    uint32_t sceneId;
};

struct PrimitiveMeshCreatedEvent : Event
{
    int primitive_type;
    glm::vec3 size;
    glm::ivec3 segments;
    int lod;
};

struct GeometryVisibilityChangedEvent : Event
{
    uint32_t entityId;
    bool visible;
};

struct EntityMovedEvent : Event
{
    uint32_t entityId;
};

struct RendererChangedEvent : Event
{
    bool useRaytracing;
};

class EventBus
{
public:
    static EventBus& Get()
    {
        static EventBus instance;
        return instance;
    }

    template<typename TEvent>
    void Subscribe(std::function<void(const TEvent&)> handler)
    {
        auto typeIndex = std::type_index(typeid(TEvent));
        m_handlers[typeIndex].push_back(
            [handler](const Event& e) {
                handler(static_cast<const TEvent&>(e));
            }
        );
    }

    template<typename TEvent>
    void Publish(const TEvent& event)
    {
        auto typeIndex = std::type_index(typeid(TEvent));
        auto it = m_handlers.find(typeIndex);
        if (it != m_handlers.end())
        {
            for (auto& handler : it->second)
            {
                handler(event);
            }
        }
    }

private:
    std::unordered_map<std::type_index, std::vector<std::function<void(const Event&)>>> m_handlers;
};