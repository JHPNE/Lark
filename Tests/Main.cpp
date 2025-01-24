#pragma once
#include "ECSTests/EntityTests.h"
#include "PhysicsTests/RotorVisualizationTest.h"
#include "PhysicsTests/TransformationTest.h"
#include <cstdio>
#include <gtest/gtest.h>

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}