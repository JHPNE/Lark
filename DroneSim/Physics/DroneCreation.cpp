// DroneCreation.h
#pragma once
#include "DroneData.h"
#include "Utils/MathTypes.h"
#include "World.h"

#include <cmath>

btRigidBody* createRigidBody(btDiscreteDynamicsWorld* bworld,
                             btCollisionShape* shape,
                             float mass,
                             const btVector3& position,
                             float linDamping = 0.01f,
                             float angDamping = 0.01f) {
    if (!bworld || !shape) return nullptr;
    
    btVector3 localInertia(0,0,0);
    if (mass > 0.f) {
        shape->calculateLocalInertia(mass, localInertia);
    }

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(position);

    btDefaultMotionState* motionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, localInertia);
    btRigidBody* body = new btRigidBody(rbInfo);

    if (body) {
        body->setDamping(linDamping, angDamping);
        bworld->addRigidBody(body);
    }
    return body;
}

void createMultirotorDrone(lark::physics::World& bulletWorld,
                           DroneData& drone,
                           btCollisionShape* fuselageShape,
                           btCollisionShape* rotorShape,
                           const btVector3& spawnPos) {
    auto* world = bulletWorld.getDynamicsWorld();
    if (!world || !fuselageShape || !rotorShape) return;

    // Pre-allocate vectors to prevent reallocation
    drone.body.childBodies.clear();
    drone.body.constraints.clear();
    drone.rotors.rotorThrottle.clear();
    
    drone.body.childBodies.reserve(drone.rotors.rotorCount);
    drone.body.constraints.reserve(drone.rotors.rotorCount);
    drone.rotors.rotorThrottle.reserve(drone.rotors.rotorCount);

    // Create fuselage
    drone.body.fuselageBody = createRigidBody(world, fuselageShape, 
        drone.rotors.rotorCount * 1.0f, spawnPos);
    
    if (!drone.body.fuselageBody) return;

    float angleStep = 2.f * float(lark::math::pi) / (float)drone.rotors.rotorCount;
    float radius = 1.0f + 0.1f * drone.rotors.rotorCount;
    
    // Initialize all rotors first
    for (int r = 0; r < drone.rotors.rotorCount; ++r) {
        float angle = angleStep * r;
        btVector3 rotorPos(
            spawnPos.x() + radius * std::cos(angle),
            spawnPos.y(),
            spawnPos.z() + radius * std::sin(angle)
        );
        
        btRigidBody* rotorBody = createRigidBody(world, rotorShape, 0.5f, rotorPos);
        if (!rotorBody) continue;

        drone.body.childBodies.push_back(rotorBody);
        drone.rotors.rotorThrottle.push_back(0.0f);
    }

    // Create constraints after all bodies are created
    for (size_t r = 0; r < drone.body.childBodies.size(); ++r) {
        btRigidBody* rotorBody = drone.body.childBodies[r];
        if (!rotorBody) continue;

        btTransform frameInA, frameInB;
        frameInA.setIdentity();
        frameInB.setIdentity();

        btVector3 localOffset = rotorBody->getCenterOfMassPosition() - 
                              drone.body.fuselageBody->getCenterOfMassPosition();
        frameInA.setOrigin(localOffset);

        auto* constraint = new btGeneric6DofSpring2Constraint(
            *drone.body.fuselageBody, *rotorBody, frameInA, frameInB
        );

        if (constraint) {
            for (int dof = 0; dof < 6; ++dof) {
                constraint->setLimit(dof, 0, 0);
            }
            world->addConstraint(constraint, true);
            drone.body.constraints.push_back(constraint);
        }
    }
}

void createFixedWingDrone(lark::physics::World& bulletWorld,
                          DroneData& drone,
                          btCollisionShape* fuselageShape,
                          btCollisionShape* wingShape,
                          const btVector3& spawnPos) {
    auto* world = bulletWorld.getDynamicsWorld();
    if (!world || !fuselageShape || !wingShape) return;

    // Clear and pre-allocate all vectors
    drone.body.childBodies.clear();
    drone.body.constraints.clear();
    drone.rotors.rotorThrottle.clear();

    drone.body.childBodies.reserve(2);  // Fuselage + wing
    drone.body.constraints.reserve(2);
    drone.rotors.rotorThrottle.reserve(drone.rotors.rotorCount);

    // Create fuselage with proper mass
    float totalMass = drone.aeroDynamics.wingArea * 1.5f;
    drone.body.fuselageBody = createRigidBody(world, fuselageShape, totalMass, spawnPos);
    if (!drone.body.fuselageBody) return;

    // Create wing with appropriate mass distribution
    btVector3 wingPos = spawnPos + btVector3(0, 0, -2);
    btRigidBody* wingBody = createRigidBody(
        world,
        wingShape,
        drone.aeroDynamics.wingArea * 0.5f,
        wingPos,
        0.02f,  // Increased damping for stability
        0.02f
    );

    if (!wingBody) {
        // Cleanup if wing creation fails
        world->removeRigidBody(drone.body.fuselageBody);
        delete drone.body.fuselageBody->getMotionState();
        delete drone.body.fuselageBody;
        drone.body.fuselageBody = nullptr;
        return;
    }

    drone.body.childBodies.push_back(wingBody);

    // Create constraint between wing and fuselage
    btTransform frameInA, frameInB;
    frameInA.setIdentity();
    frameInB.setIdentity();

    btVector3 localOffset = wingPos - spawnPos;
    frameInA.setOrigin(localOffset);

    auto* constraint = new btGeneric6DofSpring2Constraint(
        *drone.body.fuselageBody,
        *wingBody,
        frameInA,
        frameInB
    );

    if (constraint) {
        // Lock all DOFs but allow some minimal flexibility
        for (int i = 0; i < 6; i++) {
            constraint->setLimit(i, -0.01f, 0.01f);  // Small range for stability
            constraint->enableSpring(i, true);        // Enable spring for smooth motion
            constraint->setStiffness(i, 300.0f);      // Stiff spring
            constraint->setDamping(i, 10.0f);         // Reasonable damping
        }

        world->addConstraint(constraint, true);
        drone.body.constraints.push_back(constraint);
    }

    // Initialize control surfaces and other properties
    for (int i = 0; i < drone.rotors.rotorCount; i++) {
        drone.rotors.rotorThrottle.push_back(0.0f);
    }

    // Set collision flags for better performance
    drone.body.fuselageBody->setActivationState(DISABLE_DEACTIVATION);
    wingBody->setActivationState(DISABLE_DEACTIVATION);
}


void removeDroneFromWorld(DroneData& drone, btDiscreteDynamicsWorld* world) {
    if (!world) return;

    // 1. First remove all constraints
    for (auto* constraint : drone.body.constraints) {
        if (constraint) {
            world->removeConstraint(constraint);
            delete constraint;
        }
    }
    drone.body.constraints.clear();

    // 2. Then remove and delete all child bodies
    for (auto* body : drone.body.childBodies) {
        if (body) {
            world->removeRigidBody(body);
            delete body->getMotionState();
            delete body;
        }
    }
    drone.body.childBodies.clear();

    // 3. Finally remove and delete the fuselage
    if (drone.body.fuselageBody) {
        world->removeRigidBody(drone.body.fuselageBody);
        delete drone.body.fuselageBody->getMotionState();
        delete drone.body.fuselageBody;
        drone.body.fuselageBody = nullptr;
    }

    // 4. Clear all other arrays
    drone.rotors.rotorThrottle.clear();
}