#include "PhysicsWorld.h"

#include "Colliders/BoxCollider.h"
#include "collision/ContactSolver.h"

#include <iostream>

namespace drosim::physics {

  std::shared_ptr<RigidBody> PhysicsWorld::CreateRigidBody() {
    auto body = std::make_shared<RigidBody>();
    m_rigidBodies.push_back(body);
    return body;
  }

void PhysicsWorld::UpdateRigidBodyAABBs() {
    // For each rigid body,
    for (const auto& body : m_rigidBodies) {
      // For each Collider in that body
      for (auto& colliderPtr : body->GetColliders()) {
        // Check if it’s a BoxCollider (or you can do Sphere, etc.)
        if (auto* collider = colliderPtr.get()) {
          AABB* aabb = collider->GetAABB();
          if (!aabb) continue;

          // Debug old bounds
          glm::vec3 oldMin = aabb->minPoint;
          glm::vec3 oldMax = aabb->maxPoint;

          // Update the AABB
          collider->UpdateAABBBounds();

          // Optional: print debug info
          std::cout << "AABB for body at Y=" << body->GetPosition().y
                    << " moved from [" << oldMin.y << ", " << oldMax.y
                    << "] to [" << aabb->minPoint.y << ", " << aabb->maxPoint.y << "]\n";
        }
      }
    }

    // Finally, tell the broadphase to update its structure
    if (m_broadphase) {
      m_broadphase->Update();
    }
  }


  void PhysicsWorld::StepSimulation(float dt) {

    // 1. Integrate each body
    for (auto& rigidBody : m_rigidBodies) {
      if (rigidBody->GetInverseMass() == 0.0f) {
        continue;
      }

      // For simplicity, apply gravity to dynamic bodies

      glm::vec3 force(0.0f, -9.81f * rigidBody->GetMass(), 0.0f);
      rigidBody->ApplyForce(force, rigidBody->GetPosition());
      rigidBody->Integrate(dt);
    }

    // 2. Update AABBs in the broadphase
    UpdateRigidBodyAABBs();

    // 3. Narrowphase collision detection
    const auto& potentialPairs = m_broadphase->ComputePairs();
    std::vector<ContactInfo> contacts;
    contacts.reserve(potentialPairs.size());

    for (const auto& pair : potentialPairs) {
      Collider* colliderA = pair.first;
      Collider* colliderB = pair.second;

      // Skip static-static
      if (colliderA->GetRigidBody()->GetInverseMass() == 0.0f &&
          colliderB->GetRigidBody()->GetInverseMass() == 0.0f) {
        continue;
          }

      ContactInfo contact;
      if (GJKAlgorithm::DetectCollision(colliderA, colliderB, contact)) {
        contacts.push_back(contact);
      }
    }

    // 5. Collision resolution
    if (!contacts.empty()) {
      ContactSolver contact_solver;
      contact_solver.InitializeConstraints(contacts, dt);
      contact_solver.WarmStart();
      contact_solver.Solve();
      contacts.clear();
    }
  }
}