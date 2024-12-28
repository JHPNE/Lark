#pragma once
#include "PhysicsData.h"

namespace drosim::physics::cpu {
  void StepSimulation(PhysicsWorld& world, float dt, int solverIterations = 10);
}
