#include "World.h"
#include "WorldRegistry.h"
#include "PhysicExtension/Event/PhysicEvent.h"
#include "Utils/MathTypes.h"

namespace lark::physics
{
namespace
{
void handle_collisions() {}

void sync_physics_to_transform(physics::component &physics_comp, transform::component &transform_comp)
{
    btRigidBody* body = physics_comp.get_rigid_body();
    // auto& drone_state = physics_comp.get_drone_state();

    // Get position and orientation from Bullet
    btTransform transform = body->getWorldTransform();
    btVector3 position = transform.getOrigin();
    btQuaternion rotation = transform.getRotation();

    math::v3 pos(position[0], position[1], position[2]);
    transform_comp.set_position(pos);

    math::v4 rot(rotation[0], rotation[1], rotation[2], rotation[3]);
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

    // Set gravity (link this to WorldSettings if you like)
    m_dynamics_world->setGravity(btVector3(0, -9.81f, 0));
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

    // Update Drones and Add to bullet
    for (const auto &entity_id : active_entities)
    {
        game_entity::entity entity{entity_id};
        auto physics = entity.physics();
        auto drone = entity.drone();
        if (physics.is_valid() && drone.is_valid())
        {
            math::v3 pos, vel, ang_vel;
            math::v4 orient;
            physics.get_state(pos, orient, vel, ang_vel);

            // Sync to drone
            drone.sync_from_physics(pos, orient, vel, ang_vel);

            // Update drone dynamics
            const Eigen::Vector3f wind = this->get_wind()->update(dt,
                Eigen::Vector3f(pos.x, pos.y, pos.z));
            drone.update(dt, wind);

            // Get forces and torques from drone
            auto [torque, force] = drone.get_forces_and_torques();


            // Apply to physics body
            physics.apply_force(math::v3(force.x(), force.y(), force.z()));
            physics.apply_torque(math::v3(torque.x(), torque.y(), torque.z()));
        }

        if (physics.is_valid())
        {
            ensure_body_in_world(physics);
        }
    }

    // Update Bullet
    m_dynamics_world->stepSimulation(dt);

    // Handle Collisions
    handle_collisions();

    // Sync Transforms
    for (const auto &entity_id : active_entities)
    {
        game_entity::entity entity{entity_id};
        auto physics = entity.physics();
        auto transform = entity.transform();

        if (physics.is_valid() && transform.is_valid())
        {
            sync_physics_to_transform(physics, transform);
        }
    }
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
