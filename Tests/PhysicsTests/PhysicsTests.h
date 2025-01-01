#pragma once
#include "Physics/DroneCreation.h"
#include "Physics/DroneData.h"
#include "Physics/DroneUpdate.h"
#include "Physics/World.h"
#include <atomic>
#include <iostream>
#include <mutex>

#include <thread>

namespace drosim::physics {

class PhysicsTests {
public:
    void runTests(bool gpu) {
        droneTest(gpu);
    }

    void droneTest(bool gpu) {
        // Create shapes first so they outlive the drones
        auto fuselageShape = std::make_unique<btBoxShape>(btVector3(1.f, 0.3f, 1.f));
        auto rotorShape = std::make_unique<btCylinderShapeZ>(btVector3(0.2f, 0.05f, 0.2f));
        auto wingShape = std::make_unique<btBoxShape>(btVector3(2.f, 0.1f, 0.5f));

        // Create world after shapes
        drosim::physics::World world;
        auto* dynamicsWorld = world.getDynamicsWorld();
        if (!dynamicsWorld) return;

        // Create fleet
        DroneFleet droneFleet;

        // Create quadcopter
        {
            DroneData quad;
            quad.name = "QuadCopter";
            quad.type = DroneType::MULTIROTOR;
            quad.droneID = 0;
            quad.rotors.rotorCount = 4;
            quad.rotors.rotorMaxThrust = 50.0f;
            quad.battery.batteryCapacity = 1200.f;
            quad.battery.batteryLevel = 1200.f;
            quad.groundEffectFactor = 1.15f;  // Reasonable ground effect

            createMultirotorDrone(world, quad, fuselageShape.get(), rotorShape.get(), btVector3(0, 5, 0));
            if (quad.body.fuselageBody) {  // Only add if creation successful
                droneFleet.drones.push_back(std::move(quad));
            }
        }

        // Create fixed-wing
        {
            DroneData winged;
            winged.name = "FixedWing1";
            winged.type = DroneType::FIXED_WING;
            winged.droneID = 1;
            winged.aeroDynamics.wingArea = 2.5f;
            winged.aeroDynamics.wingSpan = 4.f;
            winged.rotors.rotorCount = 1;
            winged.rotors.rotorMaxThrust = 100.0f;
            winged.battery.batteryCapacity = 1500.f;
            winged.battery.batteryLevel = 1500.f;

            createFixedWingDrone(world, winged, fuselageShape.get(), wingShape.get(), btVector3(10, 5, 0));
            if (winged.body.fuselageBody) {  // Only add if creation successful
                droneFleet.drones.push_back(std::move(winged));
            }
        }

        // Run simulation
        const float fixedTimeStep = 1.f / 240.f;  // Smaller timestep for stability
        const int maxSubSteps = 4;  // Allow multiple substeps for stability
        float accumulatedTime = 0.0f;
        const float renderTimeStep = 1.f / 60.f;

        for (int frame = 0; frame < 1200 && !droneFleet.drones.empty(); ++frame) {
            accumulatedTime += renderTimeStep;

            // Update drone systems
            updateDroneSystem(droneFleet, fixedTimeStep);

            // Step physics with substeps
            world.getDynamicsWorld()->stepSimulation(accumulatedTime, maxSubSteps, fixedTimeStep);
            accumulatedTime = 0.0f;

            // Logging
            if (frame % 60 == 0) {
                for (const auto& drone : droneFleet.drones) {
                    if (drone.body.fuselageBody) {
                        const btVector3& pos = drone.body.fuselageBody->getCenterOfMassPosition();
                        std::cout << "[" << drone.name << "] ID=" << drone.droneID
                                << " Pos=(" << pos.x() << "," << pos.y() << "," << pos.z() << ")"
                                << " Battery=" << drone.battery.batteryLevel << "\n";
                    }
                }
                std::cout << "----------------------\n";
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        // Cleanup drones first
        for (auto& drone : droneFleet.drones) {
            removeDroneFromWorld(drone, dynamicsWorld);
        }
        droneFleet.drones.clear();

        // Shapes are automatically cleaned up when their unique_ptrs go out of scope
    }
};

} // namespace drosim::physics