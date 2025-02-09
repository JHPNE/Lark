#include "UtilTests/LoggingTest.h"
#include "PhysicsTests/MultirotorTest.h"
#include "PhysicsTests/ControlTest.h"
#include <gtest/gtest.h>

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}