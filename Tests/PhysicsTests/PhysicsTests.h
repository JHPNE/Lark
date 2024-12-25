#pragma once
#include <memory>
#include <string>
#include <ctime>
#include <iostream>
#include "Physics/CPU/PhysicsWorld.h"
#include "Physics/CPU/Colliders/BoxCollider.h"
#include "Physics/CPU/Colliders/SphereCollider.h"

using namespace drosim::physics;

class PhysicsTests {
public:
    void runTests(bool gpu) {
        collisionTest(gpu);
    }

    void collisionTest(bool gpu) {
        PhysicsWorld world;

        // Create falling box
        auto boxBody = world.CreateRigidBody();
        auto boxCollider = std::make_unique<BoxCollider>(glm::vec3(0.5f));

        // Store collider pointer before moving it
        Collider* boxColliderPtr = boxCollider.get();

        // Add collider to body (transfers ownership)
        boxBody->AddCollider(*boxCollider);

        // Setup box properties
        boxBody->SetPosition(glm::vec3(0.0f, 5.0f, 0.0f));
        boxBody->SetVelocity(glm::vec3(1.0f, 0.0f, 0.0f));
        boxBody->SetMass(1.0f);

        // Create ground plane
        auto groundBody = world.CreateRigidBody();
        auto groundCollider = std::make_unique<BoxCollider>(glm::vec3(10.0f, 0.1f, 10.0f));

        // Store ground collider pointer
        Collider* groundColliderPtr = groundCollider.get();

        // Add ground collider to body
        groundBody->AddCollider(*groundCollider);
        groundBody->SetPosition(glm::vec3(0.0f, 0.0f, 0.0f));
        groundBody->SetMass(0.0f); // Static body

        // Add colliders' AABBs to broadphase
        world.AddToAABBTree(boxColliderPtr->GetAABB());
        world.AddToAABBTree(groundColliderPtr->GetAABB());

        // Simulation Parameters
        float dt = 1.0f/60.0f;
        float totalTime = 0.0f;
        int maxSteps = 300;

        std::cout << "Starting physics simulation...\n";
        std::cout << "Time(s), PosX, PosY, PosZ, VelX, VelY, VelZ\n";

        for (int step = 0; step < maxSteps; step++) {
            // Get current state for validation
            glm::vec3 pos = boxBody->GetPosition();
            glm::vec3 vel = boxBody->GetVelocity();

            // Validate position
            if (glm::any(glm::isnan(pos)) || glm::any(glm::isinf(pos))) {
                std::cout << "Invalid box position detected at step " << step << "\n";
                return;
            }

            // Step simulation (this will update AABBs internally)
            world.StepSimulation(dt);
            totalTime += dt;

            // Print state every 10 frames
            if (step % 10 == 0) {
                pos = boxBody->GetPosition();
                vel = boxBody->GetVelocity();

                std::cout << totalTime << ", "
                         << pos.x << ", " << pos.y << ", " << pos.z << ", "
                         << vel.x << ", " << vel.y << ", " << vel.z << "\n";
            }

            // Optional: Break if box has settled
            if (glm::length(vel) < 0.01f && pos.y <= 0.5f) {
                std::cout << "Box has settled, ending simulation.\n";
                break;
            }
        }
    }
};;