#include "PhysicsWorld.h"

namespace drosim::physics {

  std::shared_ptr<RigidBody> PhysicsWorld::CreateRigidBody() {
    auto body = std::make_shared<RigidBody>();
    m_rigidBodies.push_back(body);
    return body;
  }

 void PhysicsWorld::UpdateRigidBodyAABBs() {
    // For each rigid body
    for (const auto& body : m_rigidBodies) {
        // Skip static bodies as their AABBs don't change
        if (body->GetInverseMass() == 0.0f) {
            continue;
        }

        // Update AABBs for each collider
        for (auto& collider : body->GetColliders()) {
            AABB* aabb = collider.GetAABB();
            if (!aabb) continue;

            // Reset AABB to empty
            aabb->minPoint = glm::vec3(std::numeric_limits<float>::max());
            aabb->maxPoint = glm::vec3(-std::numeric_limits<float>::max());

            // Sample support points along principal axes
            const glm::vec3 axes[6] = {
                glm::vec3(1, 0, 0), glm::vec3(-1, 0, 0),
                glm::vec3(0, 1, 0), glm::vec3(0, -1, 0),
                glm::vec3(0, 0, 1), glm::vec3(0, 0, -1)
            };

            // Find support points in all 6 principal directions
            for (const auto& dir : axes) {
                // Convert direction to local space
                glm::vec3 localDir = body->GlobalToLocalVec(dir);

                // Get support point in local space
                glm::vec3 support = collider.Support(localDir);

                // Transform to world space
                glm::vec3 worldSupport = body->LocalToGlobal(support);

                // Expand AABB
                aabb->Expand(worldSupport);
            }

            // Add a small margin to prevent tunneling
            const float margin = 0.01f;
            glm::vec3 marginVec(margin);
            aabb->minPoint -= marginVec;
            aabb->maxPoint += marginVec;

            // Ensure AABB is valid after computation
            if (!aabb->IsValid()) {
                // Reset to a small valid AABB around the body's position
                glm::vec3 pos = body->GetPosition();
                aabb->minPoint = pos - glm::vec3(0.1f);
                aabb->maxPoint = pos + glm::vec3(0.1f);
            }
        }
    }
}


  void PhysicsWorld::StepSimulation(float dt) {
    for (auto& rigidBody : m_rigidBodies) {
      if (rigidBody->GetInverseMass() > 0.0f) {
        rigidBody->ApplyForce(
          glm::vec3(0.0f, -9.81f * rigidBody->GetMass(), 0.0f),
          rigidBody->GetPosition()
        );
      }

      rigidBody->Integrate(dt);
    }

    UpdateRigidBodyAABBs();

    m_broadphase->Update();

    // 4. Get collision pairs
    const auto& potentialPairs = m_broadphase->ComputePairs();

    // 5. Narrow phase collision detection
    std::vector<ContactInfo> contacts;
    contacts.reserve(potentialPairs.size());

    for (const auto& pair : potentialPairs) {
      Collider* colliderA = pair.first;
      Collider* colliderB = pair.second;

      RigidBody* bodyA = colliderA->GetRigidBody();
      RigidBody* bodyB = colliderB->GetRigidBody();

      // Skip if both bodies are static
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

    // 6. Solve contacts
    if (!contacts.empty()) {
      ContactSolver solver;
      solver.InitializeConstraints(contacts, dt);
      solver.WarmStart();
      solver.Solve();
    }
  }

}