#pragma once
#include "DroneExtension/DroneManager.h"

#include <atomic>
#include <iostream>
#include <mutex>

#include <thread>

#include "DroneExtension/Components/Fuselage.h"
#include "DroneExtension/Components/Battery.h"

namespace lark::physics {

class PhysicsTests {
public:
    void runTests(bool gpu) {
         droneTest(gpu);
        //testBulletMinimal();
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
};

} // namespace lark::physics