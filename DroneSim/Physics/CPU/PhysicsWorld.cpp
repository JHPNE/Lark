#include "PhysicsWorld.h"

namespace drosim::physics {

  std::shared_ptr<RigidBody> PhysicsWorld::CreateRigidBody() {
    auto body = std::make_shared<RigidBody>();
    m_rigidBodies.push_back(body);
    return body;
  }

  void PhysicsWorld::StepSimulation(float dt) {

    // In a production engine, you typically:
    // 1) Broad-phase collision detection (collect possible colliding pairs)
    // 2) Narrow-phase collision detection (exact contact points)
    // 3) Solve constraints / impulses
    // 4) Integrate bodies, etc.

    // For now, do the simplest approach: just integrate all bodies
    for (auto &body : m_rigidBodies) {
      body->Integrate(dt);
    }
  }
}