#pragma once
#include <memory>
#include <btBulletDynamicsCommon.h>

namespace drosim::physics {
  class World {
    public:
      World() {
        createPhysicsWorld();
      }

      ~World() {
        // Ensure world is cleaned up in correct order
        if (m_dynamicsWorld) {
          // Remove and delete all constraints
          for (int i = m_dynamicsWorld->getNumConstraints() - 1; i >= 0; i--) {
            btTypedConstraint* constraint = m_dynamicsWorld->getConstraint(i);
            m_dynamicsWorld->removeConstraint(constraint);
          }

          // Remove and delete all bodies
          for (int i = m_dynamicsWorld->getNumCollisionObjects() - 1; i >= 0; i--) {
            btCollisionObject* obj = m_dynamicsWorld->getCollisionObjectArray()[i];
            btRigidBody* body = btRigidBody::upcast(obj);
            if (body && body->getMotionState()) {
              delete body->getMotionState();
            }
            m_dynamicsWorld->removeCollisionObject(obj);
            delete obj;
          }
        }
      }

      btDiscreteDynamicsWorld* getDynamicsWorld() const { return m_dynamicsWorld.get(); }
      void stepSimulation(float deltaTime);

    private:
      std::unique_ptr<btBroadphaseInterface>               m_broadphase;
      std::unique_ptr<btDefaultCollisionConfiguration>     m_collisionConfig;
      std::unique_ptr<btCollisionDispatcher>               m_dispatcher;
      std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
      std::unique_ptr<btDiscreteDynamicsWorld>             m_dynamicsWorld;

      void createPhysicsWorld() {
        m_broadphase = std::make_unique<btDbvtBroadphase>();
        m_collisionConfig = std::make_unique<btDefaultCollisionConfiguration>();
        m_dispatcher = std::make_unique<btCollisionDispatcher>(m_collisionConfig.get());
        m_solver = std::make_unique<btSequentialImpulseConstraintSolver>();

        m_dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
            m_dispatcher.get(),
            m_broadphase.get(),
            m_solver.get(),
            m_collisionConfig.get()
        );

        m_dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));

        // Important: Set internal tick callback to nullptr to prevent race conditions
        m_dynamicsWorld->setInternalTickCallback(nullptr);
        // Set small number of iterations for stability
        m_dynamicsWorld->getSolverInfo().m_numIterations = 10;
      }
  };
}
