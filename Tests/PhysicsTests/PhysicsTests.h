#pragma once
#include "DroneExtension/DroneManager.h"

#include <atomic>
#include <iostream>
#include <mutex>

#include <thread>

#include "DroneExtension/Components/Battery.h"
#include "DroneExtension/Components/Fuselage.h"
#include <DroneExtension/Components/Rotor.h>

namespace lark::physics {

class PhysicsTests {
public:
    void runTests(bool gpu) {
         //droneTest(gpu);
        //testBulletMinimal();
        rotorPhysicsTest();
    }

    // Helper function to create a simple rigid body
    btRigidBody* createSimpleBody(btDiscreteDynamicsWorld* world,
                                  btCollisionShape* shape,
                                  float mass,
                                  const btVector3& position) {
        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(position);

        btVector3 localInertia(0, 0, 0);
        if (mass != 0.f) {
            shape->calculateLocalInertia(mass, localInertia);
        }

        btDefaultMotionState* motionState = new btDefaultMotionState(transform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, localInertia);
        btRigidBody* body = new btRigidBody(rbInfo);

        world->addRigidBody(body);
        return body;
    }

    void testBulletMinimal() {
        // Create collision configuration
        btDefaultCollisionConfiguration collisionConfig;
        btCollisionDispatcher dispatcher(&collisionConfig);
        btDbvtBroadphase broadphase;
        btSequentialImpulseConstraintSolver solver;
        btDiscreteDynamicsWorld dynamicsWorld(&dispatcher, &broadphase, &solver, &collisionConfig);
        dynamicsWorld.setGravity(btVector3(0, -9.81f, 0));

        // Create ground shape and body
        btBoxShape groundShape(btVector3(50, 1, 50));
        btRigidBody* groundBody = createSimpleBody(&dynamicsWorld, &groundShape, 0.f, btVector3(0, -1, 0));

        // Create dynamic shape and body
        btBoxShape dynamicShape(btVector3(1, 1, 1));
        btRigidBody* dynamicBody = createSimpleBody(&dynamicsWorld, &dynamicShape, 1.f, btVector3(0, 10, 0));

        // Run simulation
        const float timeStep = 1.f / 60.f;
        for (int i = 0; i < 300; ++i) {
            dynamicsWorld.stepSimulation(timeStep);

            btTransform trans;
            dynamicBody->getMotionState()->getWorldTransform(trans);
            std::cout << "Frame " << i << ": Position = ("
                      << trans.getOrigin().getX() << ", "
                      << trans.getOrigin().getY() << ", "
                      << trans.getOrigin().getZ() << ")\n";

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        // Cleanup
        dynamicsWorld.removeRigidBody(dynamicBody);
        delete dynamicBody->getMotionState();
        delete dynamicBody;

        dynamicsWorld.removeRigidBody(groundBody);
        delete groundBody->getMotionState();
        delete groundBody;
    }

    void droneTest(bool gpu) {
        // Create standard fuselage info
        fuselage::init_info fuselageInfo;

        // Create entity info and set fuselage
        drone_entity::entity_info info{};
        info.fuselage = &fuselageInfo;

        // Create drone entity
        auto entity = lark::drone_entity::create(info);
        assert(entity.is_valid());
        printf("Drone Entity created!");

        // standard battery info
        battery::init_info batteryInfo;
        info.battery = &batteryInfo;

        lark::drone_entity::addDroneComponent(entity.get_id(), drone_data::BodyType::BATTERY, info);
        assert(entity.is_valid());
        printf("Added Battery to Drone Entity!");
    }

    void rotorPhysicsTest() {
        // Physics world setup
        btDefaultCollisionConfiguration collisionConfig;
        btCollisionDispatcher dispatcher(&collisionConfig);
        btDbvtBroadphase broadphase;
        btSequentialImpulseConstraintSolver solver;
        btDiscreteDynamicsWorld dynamicsWorld(&dispatcher, &broadphase, &solver, &collisionConfig);
        dynamicsWorld.setGravity(btVector3(0, -9.81f, 0));

        // Create the drone entity with a rotor
        fuselage::init_info fuselageInfo;
        rotor::init_info rotorInfo;

        // Set up realistic rotor parameters
        rotorInfo.bladeRadius = 0.127f;  // ~5 inch propeller
        rotorInfo.bladePitch = 0.175f;   // ~10 degrees in radians
        rotorInfo.bladeCount = 2;        // Standard dual-blade propeller
        rotorInfo.airDensity = 1.225f;   // Sea level air density
        rotorInfo.discArea = glm::pi<float>() * rotorInfo.bladeRadius * rotorInfo.bladeRadius;
        rotorInfo.liftCoefficient = 0.4f;
        rotorInfo.mass = 0.025f;         // 25g
        rotorInfo.rotorNormal = btVector3(0, 1, 0); // Upward thrust
        rotorInfo.position = btVector3(0, 5, 0);    // Starting position
        rotorInfo.powerConsumption = 0.0f;
        rotorInfo.currentRPM = 0.0f;
                // Create a simple box shape for the rotor
        btBoxShape* rotorShape = new btBoxShape(btVector3(rotorInfo.bladeRadius, 0.01f, rotorInfo.bladeRadius));

        // Create and add the rigid body to the physics world with damping
        btRigidBody* rotorBody = createSimpleBody(&dynamicsWorld, rotorShape, rotorInfo.mass, rotorInfo.position);
        rotorBody->setDamping(0.1f, 0.3f);  // Linear and angular damping

        // Enable rotation around y-axis only (for testing)
        rotorBody->setAngularFactor(btVector3(0, 1, 0));

        rotorInfo.rigidBody = rotorBody;

        // Create entity info and set components
        drone_entity::entity_info info{};
        info.fuselage = &fuselageInfo;
        info.rotor = &rotorInfo;

        // Create drone entity
        auto entity = drone_entity::create(info);
        assert(entity.is_valid());

        // Get the rotor component
        auto rotorComponent = entity.rotor();
        assert(rotorComponent.is_valid());



        // Set initial RPM
        rotorComponent.set_rpm(5000.0f);

        // Run simulation
        const float timeStep = 1.f / 60.f;
        const int maxSteps = 300; // 5 seconds at 60fps

        std::cout << "Starting rotor physics test..." << std::endl;
        std::cout << "Rotor mass: " << rotorInfo.mass << " kg" << std::endl;
        std::cout << "Initial RPM: 5000.0" << std::endl;

        for (int i = 0; i < maxSteps; ++i) {
            // Calculate and apply rotor forces
            rotorComponent.calculate_forces(timeStep);

            // Step the physics simulation
            dynamicsWorld.stepSimulation(timeStep);

            // Log position and forces every 60 frames (once per second)
            if (i % 60 == 0) {
                btTransform trans;
                rotorBody->getMotionState()->getWorldTransform(trans);

                btVector3 velocity = rotorBody->getLinearVelocity();
                btVector3 angularVel = rotorBody->getAngularVelocity();

                std::cout << "Frame " << i << ":\n"
                          << "Position = (" << trans.getOrigin().getX() << ", "
                          << trans.getOrigin().getY() << ", "
                          << trans.getOrigin().getZ() << ")\n"
                          << "Linear Velocity = (" << velocity.getX() << ", "
                          << velocity.getY() << ", " << velocity.getZ() << ")\n"
                          << "Angular Velocity = (" << angularVel.getX() << ", "
                          << angularVel.getY() << ", " << angularVel.getZ() << ")\n"
                          << "Thrust = " << rotorComponent.get_thrust() << " N\n"
                          << "Power = " << rotorComponent.get_power_consumption() << " W\n"
                          << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }

        // Cleanup
        dynamicsWorld.removeRigidBody(rotorBody);
        delete rotorBody->getMotionState();
        delete rotorBody;
        delete rotorShape;

        // Clean up the drone entity
        drone_entity::remove(entity.get_id());

        std::cout << "Rotor physics test completed." << std::endl;
    }
};

} // namespace lark::physics