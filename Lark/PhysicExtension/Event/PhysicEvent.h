#pragma once
#include <functional>
#include <vector>
#include <unordered_map>
#include <typeindex>
#include <memory>
#include <string>

// Base event class
struct PhysicsEvent
{
    virtual ~PhysicsEvent() = default;
};

// Specific events
struct PhysicObjectCreated : PhysicsEvent
{
    btRigidBody* body;
};

struct PhysicObjectRemoved : PhysicsEvent
{
    btRigidBody* body;
};

class PhysicEventBus
{
public:
    static PhysicEventBus& Get()
    {
        static PhysicEventBus instance;
        return instance;
    }

    template<typename TEvent>
    void Subscribe(std::function<void(const TEvent&)> handler)
    {
        auto typeIndex = std::type_index(typeid(TEvent));
        m_handlers[typeIndex].push_back(
            [handler](const PhysicsEvent& e) {
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
    std::unordered_map<std::type_index, std::vector<std::function<void(const PhysicsEvent&)>>> m_handlers;
};