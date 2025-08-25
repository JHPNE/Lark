#include "PhysicsTests/MathTest.h"
#include "PhysicsTests/DroneDynamicsTest.h"
#include "PhysicsTests/ControllerTest.h"
#include <gtest/gtest.h>

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}