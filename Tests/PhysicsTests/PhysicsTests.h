#pragma once
#include "Physics/CPU-Compute/BodySystem.h"
#include "Physics/CPU-Compute/ColliderSystem.h"
#include "Physics/CPU-Compute/ConstraintSystem.h"
#include "Physics/CPU-Compute/PhysicsSystem.h"
#include <ctime>
#include <iostream>
#include <memory>
#include <string>

#include <iomanip>

namespace drosim::physics {


class PhysicsTests {
public:
    void runTests(bool gpu) {
        collisionTest(gpu);
    }

    void collisionTest(bool gpu) {
        cpu::PhysicsWorld world;
        uint32_t groundBody = cpu::CreateBody(world, glm::vec3(0, 0, 0), 0.f);
        cpu::CreateBoxCollider(world, groundBody, glm::vec3(10.f, 0.5, 10.f));

        for (int i = 0; i < 5; i++) {
            float x = (i - 2) * 1.5f;
            float y = 5.f + i * 0.5f;
            uint32_t bodyIdx = cpu::CreateBody(world, glm::vec3(x, y, 0), 1.f);
            cpu::CreateSphereCollider(world, bodyIdx, 0.5f);
        }

        if (world.bodyPool.Size() > 2) {
            cpu::CreateDistanceConstraint(
                world,
                1,
                2,
                glm::vec3(0.f),
                glm::vec3(0.f),
                1.5f
            );
        }

        float dt = 1.f / 60.f;
        float totalTime = 0.f;

        while (totalTime < 3.f) {
            StepSimulation(world, dt, 10);
            totalTime += dt;

            // Print position of sphere #1 every 0.5s
            if (std::fmod(totalTime, 0.5f) < dt) {
                if (world.bodyPool.Size() > 1) {
                    for (int i = 0; i < world.bodyPool.Size() - 1; i++) {
                        glm::vec3 pos = GetBodyPosition(world, i);
                        std::cout << std::fixed << std::setprecision(2)
                              << "Time: " << totalTime << " s, Sphere #" << i <<" Pos: "
                              << "(" << pos.x << ", " << pos.y << ", " << pos.z << ")\n";
                    }
                }
            }
        }
    }

};
};;