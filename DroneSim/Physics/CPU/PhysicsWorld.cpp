#include "PhysicsWorld.h"

namespace drosim::physics {

  std::shared_ptr<RigidBody> PhysicsWorld::CreateRigidBody() {
    auto body = std::make_shared<RigidBody>();
    m_rigidBodies.push_back(body);
    return body;
  }

  void PhysicsWorld::StepSimulation(float dt) {

    // broadphase
    m_broadphase->Update();

    // collision pairs
    const auto& potentialPairs = m_broadphase->ComputePairs();

    // narrow phase
    std::vector<ContactInfo> contacts;

    for (const auto& pair : potentialPairs) {
      Collider* colliderA = pair.first;
      Collider* colliderB = pair.second;

      RigidBody* bodyA = colliderA->GetRigidBody();
      RigidBody* bodyB = colliderB->GetRigidBody();

      if (bodyA->GetInverseMass() == 0.0f && bodyB->GetInverseMass() == 0.0f) {
        continue;
      }

      ContactInfo contact;
      if (GJKAlgorithm::DetectCollision(colliderA, colliderB, contact)) {
        contact.bodyA = bodyA;
        contact.bodyB = bodyB;
        contacts.push_back(contact);
      }
    }

    // solve constraints
    if (!contacts.empty()) {
      ContactSolver solver;
      solver.InitializeConstraints(contacts, dt);
      solver.WarmStart();
      solver.Solve();
    }

    // integrate velocities and apply forces
    for (auto& rigidBody : m_rigidBodies) {
      if (rigidBody->GetInverseMass() > 0.0f) {
        rigidBody->ApplyForce(
          glm::vec3(0.0f, -9.81f * rigidBody->GetMass(), 0.0f),
          rigidBody->GetPosition()
        );
      }

      rigidBody->Integrate(dt);
    }
  }
}