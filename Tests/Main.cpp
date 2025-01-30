#pragma once
#include "ECSTests/EntityTests.h"
#include "PhysicsTests/RotorVisualizationTest.h"
#include "PhysicsTests/TransformationTest.h"
#include "PhysicsTests/IsaModelTest.h"
#include "PhysicsTests/BladeFlappingTest.h"

#include <cstdio>
#include "PhysicsTests/ExtendedRotorTests.h"
#include <gtest/gtest.h>
#include <string>

void runRotorVisualizationTest() {
    // Create test configuration
    lark::physics::RotorTestConfig config;
    config.visual_mode = false;
    config.simulation_speed = 10.0f;
    config.target_rpm = 5000.0f;
    config.test_duration = 6000.0f;

    // Create and run the visualization test
    try {
        std::cout << "\nStarting Rotor Visualization Test...\n" << std::endl;
        lark::physics::RotorVisualizationTest test(config);
        test.run();
        std::cout << "\nRotor Visualization Test completed successfully.\n" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error in Rotor Visualization Test: " << e.what() << std::endl;
    }
}

int main(int argc, char** argv) {
    // Check if we should run visualization test
    bool runVisualization = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--run-visualization") {
            runVisualization = true;
            // Remove this argument to not interfere with Google Test
            for (int j = i; j < argc - 1; ++j) {
                argv[j] = argv[j + 1];
            }
            --argc;
            break;
        }
    }

    if (runVisualization) {
        runRotorVisualizationTest();
        return 0;
    }

    // Run Google Tests normally
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}