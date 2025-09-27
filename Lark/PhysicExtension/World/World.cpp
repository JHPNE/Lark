#include "World.h"
#include "WorldRegistry.h"
#include "PhysicExtension/Event/PhysicEvent.h"
#include "Utils/MathTypes.h"

namespace lark::physics
{
namespace
{
void handle_collisions() {}

void sync_drone_to_transform(drone::component &drone_comp, transform::component &transform_comp)
{
    // Get drone state
    drone::DroneState state = drone_comp.get_state();

    // Update position from drone
    math::v3 pos(state.position.x(), state.position.y(), state.position.z());
    transform_comp.set_position(pos);

    // Update rotation from drone attitude quaternion
    // DroneState stores as [x,y,z,w]
    math::v4 rot(state.attitude.x(), state.attitude.y(), state.attitude.z(), state.attitude.w());
    transform_comp.set_rotation(rot);
}

} // namespace

World::World()
{
    // Bullet setup
    m_collision_config = new btDefaultCollisionConfiguration();
    m_dispatcher = new btCollisionDispatcher(m_collision_config);
    m_broadphase = new btDbvtBroadphase();
    m_solver = new btSequentialImpulseConstraintSolver();

    m_dynamics_world =
        new btDiscreteDynamicsWorld(m_dispatcher, m_broadphase, m_solver, m_collision_config);

    auto pending = WorldRegistry::instance().take_pending_settings();
    if (pending.wind)
    {
        this->set_wind(pending.wind);
    }

    if (pending.gravity)
    {
        m_dynamics_world->setGravity(pending.gravity);
    }

    m_dynamics_world->setGravity(pending.gravity);
    // Set gravity (link this to WorldSettings if you like)
    WorldRegistry::instance().set_active_world(this);
}

World::~World()
{
    // Unregister if we're the active world
    if (WorldRegistry::instance().get_active_world() == this)
    {
        WorldRegistry::instance().set_active_world(nullptr);
    }

    // Clean up all rigid bodies before destroying the world
    cleanup_all_bodies();

    delete m_dynamics_world;
    delete m_solver;
    delete m_broadphase;
    delete m_dispatcher;
    delete m_collision_config;
}

void World::cleanup_all_bodies()
{
    // Remove all rigid bodies from the world
    for (int i = m_dynamics_world->getNumCollisionObjects() - 1; i >= 0; i--)
    {
        btCollisionObject* obj = m_dynamics_world->getCollisionObjectArray()[i];
        btRigidBody* body = btRigidBody::upcast(obj);
        if (body && body->getMotionState())
        {
            delete body->getMotionState();
        }
        m_dynamics_world->removeCollisionObject(obj);
        delete obj;
    }
}

void World::update(f32 dt)
{
    // Early exit if world not properly initialized
    if (!m_dynamics_world)
    {
        printf("No World");
        return;
    }

    const auto &active_entities = game_entity::get_active_entities();

    // Frame counter for debug output
    static int frame_count = 0;
    frame_count++;

    // Update Drones and Add to bullet
    for (const auto &entity_id : active_entities)
    {
        game_entity::entity entity{entity_id};
        auto drone = entity.drone();
        auto transform = entity.transform();
        if (drone.is_valid())
        {
            // Get current drone state
            drone::DroneState current_state = drone.get_state();

            // Get wind at current position
            auto wind = this->get_wind();
            Eigen::Vector3f wind_vec = wind->update(dt, current_state.position);

            // Update drone dynamics (this runs the full simulation step)
            drone.update(dt, wind_vec);

            // Sync updated drone state to transform
            sync_drone_to_transform(drone, transform);

            // Debug output every second (assuming 60 FPS)
            if (frame_count % 60 == 0) {
                drone::DroneState state = drone.get_state();
                printf("Drone State - Pos: (%.2f, %.2f, %.2f) Vel: (%.2f, %.2f, %.2f) "
                       "Orient: (%.2f, %.2f, %.2f, %.2f)\n",
                       state.position.x(), state.position.y(), state.position.z(),
                       state.velocity.x(), state.velocity.y(), state.velocity.z(),
                       state.attitude.x(), state.attitude.y(), state.attitude.z(), state.attitude.w());

                // Also show rotor speeds for debugging
                printf("  Rotor Speeds: [%.1f, %.1f, %.1f, %.1f] rad/s\n",
                       state.rotor_speeds[0], state.rotor_speeds[1],
                       state.rotor_speeds[2], state.rotor_speeds[3]);
            }
        }

        // For non-drone entities with physics, we can still register them with Bullet
        // but skip the simulation for now
        auto physics = entity.physics();
        if (physics.is_valid() && !drone.is_valid())
        {
            ensure_body_in_world(physics);
        }
    }

    // Skip Bullet simulation for now
    // m_dynamics_world->stepSimulation(dt);

    // Handle any custom collisions if needed
    handle_collisions();
}

void World::ensure_body_in_world(physics::component& physics_comp)
{
    btRigidBody* body = physics_comp.get_rigid_body(); // New method
    if (body && !body->isInWorld())
    {
        m_dynamics_world->addRigidBody(body);
    }
}
} // namespace lark::physics
