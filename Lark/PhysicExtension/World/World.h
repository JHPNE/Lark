#pragma once
#include "Components/Entity.h"
#include <btBulletDynamicsCommon.h>
#include "PhysicExtension/Utils/Wind.h"

namespace lark::game_entity { class entity; }

namespace lark::physics
{

class World
{
  public:

    World();
    ~World();

    void update(f32 dt);

    btDiscreteDynamicsWorld *dynamics_world() { return m_dynamics_world; }
    void set_wind(std::shared_ptr<drone::Wind> wind) { m_wind = wind; }
    drone::Wind* get_wind() const { return m_wind.get(); }

  private:

    void ensure_body_in_world(physics::component& physics_comp);
    void cleanup_all_bodies();

    // Bullet Physics
    btDefaultCollisionConfiguration *m_collision_config;
    btCollisionDispatcher *m_dispatcher;
    btBroadphaseInterface *m_broadphase;
    btSequentialImpulseConstraintSolver *m_solver;
    btDiscreteDynamicsWorld *m_dynamics_world;

    // Wind
    std::shared_ptr<drone::Wind> m_wind;
};

} // namespace lark::physics