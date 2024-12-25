#pragma once

#include "AABBTree.h"
#include "RigidBody.h"
#include "collision/ContactSolver.h"
#include "collision/GJK.h"
#include <memory>
#include <vector>

#include <stdexcept>

namespace drosim::physics {
  class PhysicsWorld {
    public:
      PhysicsWorld() {
        m_broadphase = std::make_unique<AABBTree>();
      };
      ~PhysicsWorld() = default;

      std::shared_ptr<RigidBody> CreateRigidBody();

      void StepSimulation(float dt);

      void AddToAABBTree(AABB* aabb) {
        if (!m_broadphase) {
          throw std::runtime_error("Broadphase not initialized!");
        }
        if (!aabb) {
          throw std::runtime_error("Null AABB pointer!");
        }
        m_broadphase->Add(aabb);
      }

    private:
      std::vector<std::shared_ptr<RigidBody>> m_rigidBodies;
      std::unique_ptr<Broadphase> m_broadphase;
  };
};

/*
Physics::PhysicsWorld world;

    // Create a rigid body
    auto rigidBody = world.CreateRigidBody();

    // Create a convex mesh shape from some geometry
    MyMeshShape myShape( pass in vertices, etc. );
Physics::Collider collider(myShape);

// Add collider to the rigid body
rigidBody->AddCollider(collider);

// Optionally set initial position, velocities, etc.
rigidBody->SetPosition(glm::vec3(0.0f, 10.0f, 0.0f));
rigidBody->SetVelocity(glm::vec3(0.0f, 0.0f, 0.0f));

// Main loop
float dt = 1.0f / 60.0f;  // 60fps
for (int frame = 0; frame < 600; ++frame)
{
  // Apply gravity, for example
  rigidBody->ApplyForce(glm::vec3(0.0f, -9.81f * rigidBody->GetMass(), 0.0f),
                        rigidBody->LocalToGlobal(rigidBody->GetLocalCentroid()));

  // Step the simulation
  world.StepSimulation(dt);

  // Print out the updated centroid
  auto pos = rigidBody->GetPosition();
  std::cout << "Frame: " << frame << " Position = ("
            << pos.x << ", " << pos.y << ", " << pos.z << ")\n";
}
*/