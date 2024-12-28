#pragma once
#include "PhysicsData.h"

namespace drosim::physics::cpu {
  void CreateDistanceConstraint(PhysicsWorld& world, uint32_t bodyA, uint32_t bodyB,
                                const glm::vec3& localAnchorA, const glm::vec3& localAnchorB,
                                float restLength);

  void SolveConstraints(PhysicsWorld& world, float dt, int iterations);
}
