#pragma once
#include <btBulletDynamicsCommon.h>
#include "Components/Entity.h"

namespace lark::physics {

    class World {
    public:
        World();
        ~World();

        void update(f32 dt);

        btDiscreteDynamicsWorld* dynamics_world() { return m_dynamics_world; }

    private:
        btDefaultCollisionConfiguration* m_collision_config;
        btCollisionDispatcher* m_dispatcher;
        btBroadphaseInterface* m_broadphase;
        btSequentialImpulseConstraintSolver* m_solver;
        btDiscreteDynamicsWorld* m_dynamics_world;
    };

}