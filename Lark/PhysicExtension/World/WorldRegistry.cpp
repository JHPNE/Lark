#include "WorldRegistry.h"
#include "World.h"
#include "PhysicExtension/Event/PhysicEvent.h"

namespace lark::physics
{
    WorldRegistry::WorldRegistry()
    {
        SubscribeToEvents();
    }

    WorldRegistry::~WorldRegistry() = default;

    btDiscreteDynamicsWorld* WorldRegistry::get_dynamics_world()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (active_world)
        {
            return active_world->dynamics_world();
        }
        return nullptr;
    }

    void WorldRegistry::add_rigid_body(btRigidBody* body)
    {
        if (!body) return;

        auto* dynamics_world = get_dynamics_world();
        if (dynamics_world && !body->isInWorld())
        {
            dynamics_world->addRigidBody(body);
        }
    }

    void WorldRegistry::remove_rigid_body(btRigidBody* body)
    {
        if (!body) return;

        auto* dynamics_world = get_dynamics_world();
        if (dynamics_world && body->isInWorld())
        {
            dynamics_world->removeRigidBody(body);
        }
    }

    void WorldRegistry::SubscribeToEvents() {
       PhysicEventBus::Get().Subscribe<PhysicObjectCreated>(
 [this](const PhysicObjectCreated& e) {
            add_rigid_body(e.body);
           }
       );

       PhysicEventBus::Get().Subscribe<PhysicObjectRemoved>(
 [this](const PhysicObjectRemoved& e) {
            remove_rigid_body(e.body);
           }
       );
    }
}
