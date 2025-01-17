#pragma once
#include "ECSTests/EntityTests.h"
#include "PhysicsTests/RotorVisualizationTest.h"

#include <cstdio>

int main() {
  printf("Entity Tests \n");
  EntityTests entity_tests;
  entity_tests.runTests();

  printf("Physics Tests \n");
  try {
    physics::RotorTestConfig config;
    config.visual_mode = true;
    config.simulation_speed = 1.0f;

    lark::physics::RotorVisualizationTest test(config);
    test.run();
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 1;
  }

  return 0;
}