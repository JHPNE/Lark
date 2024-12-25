#pragma once
#include <memory>
#include <string>
#include <ctime>
#include <iostream>
#include "Physics/CPU/PhysicsWorld.h"
#include "Physics/CPU/Colliders/BoxCollider.h"
#include "Physics/CPU/Colliders/SphereCollider.h"
#include "Physics/CPU/AABB.h"

using namespace drosim::physics;

class PhysicsTests {
  public:
    void runTests(bool gpu) {
      //rigidBodyTest();
      collisionTest(gpu);
    };


    void collisionTest(bool gpu) {
      PhysicsWorld world;

      // falling box
      auto boxBody = world.CreateRigidBody();
      auto boxCollider = std::make_unique<BoxCollider>(glm::vec3(0.5f));
      boxBody->AddCollider(*boxCollider);
      boxBody->SetPosition(glm::vec3(0.0f, 5.0f, 0.0f));
      boxBody->SetVelocity(glm::vec3(1.0f, 0.0f, 0.0f));
      boxBody->SetMass(1.0f);

      // create aabb for the box
      const float AABB_MARGIN = 0.1f;
      auto boxAABB = std::make_unique<AABB>();
      glm::vec3 boxPos = boxBody->GetPosition();
      glm::vec3 boxExtent = glm::vec3(0.5f) + glm::vec3(AABB_MARGIN);
      boxAABB->minPoint = boxPos - boxExtent;
      boxAABB->maxPoint = boxPos + boxExtent;
      boxAABB->colliderPtr = boxCollider.get();

      // ground plane
      auto groundBody = world.CreateRigidBody();
      auto groundCollider = std::make_unique<BoxCollider>(glm::vec3(10.0f, 0.1f, 10.0f));
      groundBody->AddCollider(*groundCollider);
      groundBody->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
      groundBody->SetMass(0.0f);

      auto groundAABB = std::make_unique<AABB>();
      groundAABB->minPoint = groundBody->GetPosition() - glm::vec3(10.0f, 0.1f, 10.0f);
      groundAABB->maxPoint = groundBody->GetPosition() + glm::vec3(10.0f, 0.1f, 10.0f);
      groundAABB->colliderPtr = groundCollider.get();
      groundAABB->userData = nullptr;

      if (!boxAABB->IsValid()) {
        std::cout << "Box AABB invalid before simulation!\n";
        return;
      }
      if (!groundAABB->IsValid()) {
        std::cout << "Ground AABB invalid before simulation!\n";
        return;
      }

      world.AddToAABBTree(boxAABB.get());
      world.AddToAABBTree(groundAABB.get());

      // Simulation Parameters
      float dt = 1.0f/60.0f;
      float totalTime = 0.0f;
      int maxSteps = 300;

      std::cout << "Starting physics Simulation ...\n";
      std::cout << "Time(s), PosX, PosY, PosZ, VelX, VelY, VelZ\n";

      for (int step = 0; step < maxSteps; step++) {
        // Get current state
        boxPos = boxBody->GetPosition();
        glm::vec3 vel = boxBody->GetVelocity();

        // Validate position before updating AABB
        if (glm::any(glm::isnan(boxPos)) || glm::any(glm::isinf(boxPos))) {
          std::cout << "Invalid box position detected at step " << step << "\n";
          return;
        }

        // Update AABB for moving box
        //boxAABB->minPoint = boxPos - boxExtent;
        //boxAABB->maxPoint = boxPos + boxExtent;

        world.StepSimulation(dt);
        totalTime += dt;

        // Print state every 10 frames
        if (step % 10 == 0) {
          glm::vec3 pos = boxBody->GetPosition();
          glm::vec3 vel = boxBody->GetVelocity();

          std::cout << totalTime << ", "
                   << pos.x << ", " << pos.y << ", " << pos.z << ", "
                   << vel.x << ", " << vel.y << ", " << vel.z << "\n";
        }

        // Optional: Break if box has settled (very low velocity)
        if (glm::length(vel) < 0.01f && boxBody->GetPosition().y <= 0.5f) {
          std::cout << "Box has settled, ending simulation.\n";
          break;
        }
      }
    }
};