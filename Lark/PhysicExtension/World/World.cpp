#include "World.h"

#include "WorldSettings.h"

namespace lark::physics
{
namespace
{
void handle_collisions() {}

void sync_physics_to_transform(physics::component &physics_comp, transform::component &transform_comp)
{
    btRigidBody &body = physics_comp.get_rigid_body();
    // auto& drone_state = physics_comp.get_drone_state();

    // Get position and orientation from Bullet
    btTransform transform = body.getWorldTransform();
    btVector3 position = transform.getOrigin();
    btQuaternion rotation = transform.getRotation();

    math::v3 pos(position[0], position[1], position[2]);
    transform_comp.set_position(pos);

    math::v4 rot(rotation[0], rotation[1], rotation[2], rotation[3]);
    transform_comp.set_rotation(rot);

    // Update drone state

    //physics_comp.set_drone_state()
    //drone_state.position = Eigen::Vector3f(position.x(), position.y(), position.z());
    //drone_state.attitude = Eigen::Vector4f(rotation.x(), rotation.y(), rotation.z(), rotation.w());
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
}

World::~World()
{
    delete m_dynamics_world;
    delete m_solver;
    delete m_broadphase;
    delete m_dispatcher;
    delete m_collision_config;
}

void World::update(f32 dt)
{
    const auto &active_entities = game_entity::get_active_entities();

    // Update Drones and Add to bullet
    for (const auto &entity_id : active_entities)
    {
        game_entity::entity entity{entity_id};
        auto physics = entity.physics();
        if (physics.is_valid())
        {
            physics.step(dt);
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
} // namespace lark::physics
