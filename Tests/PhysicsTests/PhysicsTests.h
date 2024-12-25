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

        // Create ground first
        auto groundBody = world.CreateRigidBody();
        auto groundCollider = std::make_unique<BoxCollider>(glm::vec3(10.0f, 0.1f, 10.0f));
        Collider* groundColliderPtr = groundCollider.get();

        // Initialize ground collider AABB
        AABB* groundAABB = groundColliderPtr->GetAABB();
        if (groundAABB) {
            groundAABB->userData = groundColliderPtr;
            glm::vec3 groundPos(0.0f, -0.1f, 0.0f);
            glm::vec3 groundExtents(10.0f, 0.1f, 10.0f);
            groundAABB->minPoint = groundPos - groundExtents;
            groundAABB->maxPoint = groundPos + groundExtents;
        }

        groundBody->SetPosition(glm::vec3(0.0f, -0.1f, 0.0f));
        groundBody->SetMass(0.0f);
        groundBody->SetRestitution(0.3f);
        groundBody->SetFriction(0.5f);
        groundBody->AddCollider(*groundCollider);

        // Create falling box
        auto boxBody = world.CreateRigidBody();
        auto boxCollider = std::make_unique<BoxCollider>(glm::vec3(0.5f));
        Collider* boxColliderPtr = boxCollider.get();

        // Initialize box collider AABB
        AABB* boxAABB = boxColliderPtr->GetAABB();
        if (boxAABB) {
            boxAABB->userData = boxColliderPtr;
            glm::vec3 boxPos(0.0f, 5.0f, 0.0f);
            glm::vec3 boxExtents(0.5f);
            boxAABB->minPoint = boxPos - boxExtents;
            boxAABB->maxPoint = boxPos + boxExtents;
        }

        boxBody->SetPosition(glm::vec3(0.0f, 5.0f, 0.0f));
        boxBody->SetVelocity(glm::vec3(1.0f, 0.0f, 0.0f));
        boxBody->SetMass(1.0f);
        boxBody->SetRestitution(0.3f);
        boxBody->SetFriction(0.5f);
        boxBody->AddCollider(*boxCollider);

        std::cout << "Adding colliders to AABB tree\n";
        world.AddToAABBTree(groundColliderPtr->GetAABB());
        world.AddToAABBTree(boxColliderPtr->GetAABB());

        // Verify AABB tree state
        std::cout << "Initial body positions:\n";
        std::cout << "Box: Y=" << boxBody->GetPosition().y
                  << " (AABB Y=[" << boxColliderPtr->GetAABB()->minPoint.y
                  << ", " << boxColliderPtr->GetAABB()->maxPoint.y << "])\n";
        std::cout << "Ground: Y=" << groundBody->GetPosition().y
                  << " (AABB Y=[" << groundColliderPtr->GetAABB()->minPoint.y
                  << ", " << groundColliderPtr->GetAABB()->maxPoint.y << "])\n";

        // Simulation Parameters
        float dt = 1.0f/60.0f;
        float totalTime = 0.0f;
        int maxSteps = 300;

        std::cout << "Starting physics simulation...\n";
        std::cout << "Time(s), PosX, PosY, PosZ, VelX, VelY, VelZ\n";

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

                // Debug AABB state
                std::cout << "Box AABB: Y=[" << boxColliderPtr->GetAABB()->minPoint.y
                         << ", " << boxColliderPtr->GetAABB()->maxPoint.y << "]\n";
                std::cout << "Ground AABB: Y=[" << groundColliderPtr->GetAABB()->minPoint.y
                         << ", " << groundColliderPtr->GetAABB()->maxPoint.y << "]\n";
            }

            // Check if box has settled
            if (glm::length(vel) < 0.01f && std::abs(pos.y - 0.5f) < 0.01f) {
                std::cout << "Box has settled at height: " << pos.y << "\n";
                hasSettled = true;
            }
        }
    }
};;