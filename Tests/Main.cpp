#pragma once
#include "PhysicsTests/PhysicsTests.h"
#include "ECSTests/EntityTests.h"
#include "Physics/CPU/PhysicsWorld.h"
#include <cstdio>

int main() {
  printf("Entity Tests \n");
  EntityTests entity_tests;
  entity_tests.runTests();

  printf("Physics Tests \n");
  try {
    PhysicsTests test;
    test.collisionTest(true);
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}