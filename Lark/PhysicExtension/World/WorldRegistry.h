#pragma once
#include <btBulletDynamicsCommon.h>
#include <mutex>
#include "PhysicExtension/Utils/Wind.h"

namespace lark::physics
{
    class World; // Forward declaration


    struct PendingSettings
    {
        std::shared_ptr<drone::Wind> wind;
        btVector3 gravity;
    };

    class WorldRegistry
    {
    public:
        static WorldRegistry& instance()
        {
            static WorldRegistry inst;
            return inst;
        }

        void set_active_world(World* world)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            active_world = world;
        }

        World* get_active_world()
        {
            std::lock_guard<std::mutex> lock(mutex_);
            return active_world;
        }

        // Direct access to Bullet world for component operations
        btDiscreteDynamicsWorld* get_dynamics_world();

        // Helper to safely add/remove bodies
        void add_rigid_body(btRigidBody* body);
        void remove_rigid_body(btRigidBody* body);

        void set_pending_gravity(btVector3 gravity);
        void set_pending_wind(std::shared_ptr<drone::Wind> wind);
        PendingSettings take_pending_settings() {return pending_; };


    private:
        void SubscribeToEvents();

        WorldRegistry();
        ~WorldRegistry();
        WorldRegistry(const WorldRegistry&) = delete;
        WorldRegistry& operator=(const WorldRegistry&) = delete;

        World* active_world = nullptr;
        std::mutex mutex_;
        PendingSettings pending_;
    };

} // namespace lark::physics