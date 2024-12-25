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
        Collider* boxColliderPtr = boxCollider.get();
        boxBody->AddCollider(*boxCollider);

        // Setup box properties
        boxBody->SetPosition(glm::vec3(0.0f, 5.0f, 0.0f));
        boxBody->SetVelocity(glm::vec3(1.0f, 0.0f, 0.0f));
        boxBody->SetMass(1.0f);
        boxBody->SetRestitution(0.3f);
        boxBody->SetFriction(0.5f);

        // Create ground
        auto groundBody = world.CreateRigidBody();
        auto groundCollider = std::make_unique<BoxCollider>(glm::vec3(10.0f, 0.1f, 10.0f));
        Collider* groundColliderPtr = groundCollider.get();
        groundBody->AddCollider(*groundCollider);
        groundBody->SetPosition(glm::vec3(0.0f, -0.1f, 0.0f));
        groundBody->SetMass(0.0f); // Static body
        groundBody->SetRestitution(0.3f);
        groundBody->SetFriction(0.5f);

        // Add both colliders to broadphase
        world.AddToAABBTree(boxColliderPtr->GetAABB());
        world.AddToAABBTree(groundColliderPtr->GetAABB());

        // Simulation Parameters
        float dt = 1.0f/60.0f;
        float totalTime = 0.0f;
        int maxSteps = 300;

        std::cout << "Starting physics simulation...\n";

        // Print initial state BEFORE first step
        glm::vec3 initialPos = boxBody->GetPosition();
        glm::vec3 initialVel = boxBody->GetVelocity();
        std::cout << "Initial state before simulation:\n";
        std::cout << "Position: " << initialPos.x << ", " << initialPos.y << ", " << initialPos.z << "\n";
        std::cout << "Velocity: " << initialVel.x << ", " << initialVel.y << ", " << initialVel.z << "\n\n";

        std::cout << "Time(s), PosX, PosY, PosZ, VelX, VelY, VelZ\n";
        std::cout << totalTime << ", "
                  << initialPos.x << ", " << initialPos.y << ", " << initialPos.z << ", "
                  << initialVel.x << ", " << initialVel.y << ", " << initialVel.z << "\n";

        bool hasSettled = false;
        for (int step = 0; step < maxSteps && !hasSettled; step++) {
            // Step simulation
            world.StepSimulation(dt);
            totalTime += dt;

            // Get state after stepping
            glm::vec3 pos = boxBody->GetPosition();
            glm::vec3 vel = boxBody->GetVelocity();

            // Validate position
            if (glm::any(glm::isnan(pos)) || glm::any(glm::isinf(pos))) {
                std::cout << "Invalid box position detected at step " << step << "\n";
                return;
            }

            // Print state every 10 frames
            if (step % 10 == 0) {
                std::cout << totalTime << ", "
                         << pos.x << ", " << pos.y << ", " << pos.z << ", "
                         << vel.x << ", " << vel.y << ", " << vel.z << "\n";
            }

            // Check if box has settled
            if (glm::length(vel) < 0.01f && std::abs(pos.y - 0.5f) < 0.01f) {
                std::cout << "Box has settled at height: " << pos.y << "\n";
                hasSettled = true;
            }
        }
    }
};;