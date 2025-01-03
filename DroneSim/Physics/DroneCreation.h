#pragma once
#include "World.h"
#include "DroneData.h"
#include <cmath>

btRigidBody* createRigidBody(btDiscreteDynamicsWorld* bworld,
                             btCollisionShape* shape,
                             float mass,
                             const btVector3& position,
                             float linDamping = 0.01f,
                             float angDamping = 0.01f);

void createMultirotorDrone(lark::physics::World& world,
                           DroneData& drone,
                           btCollisionShape* fuselageShape,
                           btCollisionShape* rotorShape,
                           const btVector3& spawnPos);

void createFixedWingDrone(lark::physics::World& world,
                          DroneData& drone,
                          btCollisionShape* fuselageShape,
                          btCollisionShape* wingShape,
                          const btVector3& spawnPos);

void removeDroneFromWorld(DroneData& drone, btDiscreteDynamicsWorld* world);