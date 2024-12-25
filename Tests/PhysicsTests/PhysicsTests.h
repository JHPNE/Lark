#pragma once
#include <memory>
#include <string>
#include <ctime>
#include <iostream>
#include "Physics/CPU/PhysicsStructures.h"  // Add this explicit include
#include "Physics/CPU/PhysicsWorld.h"
#include "Physics/CPU/Colliders/BoxCollider.h"
#include "Physics/CPU/Colliders/SphereCollider.h"

namespace drosim::physics {

class PhysicsTests {
public:
    void runTests(bool gpu) {
        collisionTest(gpu);
    }

    void collisionTest(bool gpu) {
        PhysicsWorld world;

        // -----------------------------------------------------
        // 1) Create ground body + collider
        // -----------------------------------------------------
        auto groundBody = world.CreateRigidBody();
        groundBody->SetPosition(glm::vec3(0.0f, -0.1f, 0.0f));
        groundBody->SetMass(0.0f);
        groundBody->SetRestitution(0.3f);
        groundBody->SetFriction(0.5f);

        // Make BoxCollider for ground, half-extents = (10,0.1,10)
        auto groundCollider = std::make_unique<BoxCollider>(glm::vec3(10.0f, 0.1f, 10.0f));
        // Grab a pointer to the raw collider *before* we move it
        Collider* groundColliderPtr = groundCollider.get();

        // Move it into the ground body
        groundBody->AddCollider(std::move(groundCollider));

        // Now we can register its AABB with the broadphase
        if (auto* aabb = groundColliderPtr->GetAABB()) {
            aabb->userData = groundColliderPtr; // ensure userData is set
            glm::vec3 groundPos = groundBody->GetPosition();
            glm::vec3 ext = glm::vec3(10.0f, 0.1f, 10.0f);
            aabb->minPoint = groundPos - ext;
            aabb->maxPoint = groundPos + ext;
            world.AddToAABBTree(aabb);
        }

        // -----------------------------------------------------
        // 2) Create falling box
        // -----------------------------------------------------
        auto boxBody = world.CreateRigidBody();
        boxBody->SetPosition(glm::vec3(0.0f, 2.0f, 0.0f));
        boxBody->SetVelocity(glm::vec3(1.0f, 0.0f, 0.0f));
        boxBody->SetMass(1.0f);
        boxBody->SetRestitution(0.3f);
        boxBody->SetFriction(0.5f);

        auto boxCollider = std::make_unique<BoxCollider>(glm::vec3(0.5f));
        Collider* boxColliderPtr = boxCollider.get();
        boxBody->AddCollider(std::move(boxCollider));

        // Register box AABB
        if (auto* aabb = boxColliderPtr->GetAABB()) {
            aabb->userData = boxColliderPtr;
            glm::vec3 pos = boxBody->GetPosition();
            glm::vec3 he = glm::vec3(0.5f);
            aabb->minPoint = pos - he;
            aabb->maxPoint = pos + he;
            world.AddToAABBTree(aabb);
        }

        // Now run your simulation
        std::cout << "Starting physics simulation...\n";
        float dt = 1.0f/60.0f;
        for (int step = 0; step < 100; step++) {
            world.StepSimulation(dt);

            glm::vec3 pos = boxBody->GetPosition();
            glm::vec3 vel = boxBody->GetVelocity();

            // Print state every 10 frames
            if (step % 10 == 0) {
                std::cout << "Step " << step << " - PosY=" << pos.y
                          << ", VelY=" << vel.y << "\n";
            }
        }
    }
};
};;