#pragma once
#include "PhysicsTests/PhysicsTests.h"
#include "ECSTests/EntityTests.h"
#include "Physics/CPU/PhysicsWorld.h"
#include <cstdio>

int main() {
  printf("Entity Tests \n");
  EntityTests entity_tests;
  entity_tests.runTests();

  /*
  printf("Physics Tests \n");
  PhysicsTests physicsTests;
  physicsTests.runTests(false);
  */

  drosim::physics::PhysicsWorld world;
  auto rigidBody = world.CreateRigidBody();
  /*drosim::physics::ConvexMeshShape shape(vertices);
  physics::Collider collider(shape);
  rigidBody->AddCollider(collider);
  */


  return 0;
}