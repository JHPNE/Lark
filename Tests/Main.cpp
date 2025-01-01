#pragma once
#include "PhysicsTests/PhysicsTests.h"
#include "ECSTests/EntityTests.h"
#include <cstdio>

int main() {
  printf("Entity Tests \n");
  EntityTests entity_tests;
  entity_tests.runTests();

  printf("Physics Tests \n");
  try {
    physics::PhysicsTests test;
    test.runTests(true);
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}