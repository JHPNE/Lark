#pragma once
#include "PhysicsTests/PhysicsTests.h"
#include "ECSTests/EntityTests.h"
#include <cstdio>

int main() {
  printf("Entity Tests \n");
  EntityTests entity_tests;
  entity_tests.runTests();

  printf("Physics Tests \n");
  PhysicsTests physicsTests;
  physicsTests.runTests();
  return 0;
}