#pragma once
#include "ECSTests/EntityTests.h"
#include "PhysicsTests/RotorVisualizationTest.h"
#include "PhysicsTests/TransformationTest.h"

#include <cstdio>

int main() {
  printf("Entity Tests \n");
  EntityTests entity_tests;
  entity_tests.runTests();

  printf("Physics Tests \n");
  try {

    physics::RotorTestConfig config;
    config.visual_mode = false;
    config.simulation_speed = 10.0f;
    config.ground_effect = false;

    lark::physics::RotorVisualizationTest rotorVisTest(config);
    rotorVisTest.run();

    /*
    physics::TransformTestConfig config;
    config.visual_mode = true;
    config.test_duration = 60.0f;

    physics::TransformationTest transformation_test(config);
    transformation_test.run();
    */
  }
  catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
    return 0;
  }

  return -1;
}