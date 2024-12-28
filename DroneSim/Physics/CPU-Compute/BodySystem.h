#pragma once
#include "PhysicsData.h"

namespace drosim::physics::cpu {
  uint32_t CreateBody(PhysicsWorld& world, const glm::vec3& pos, float mass);

  glm::vec3 GetBodyPosition(PhysicsWorld& world, uint32_t body);

  void IntegrateForces(PhysicsWorld& world, float dt);

  void IntegrateVelocities(PhysicsWorld& world, float dt);

  void UpdateSleeping(PhysicsWorld& world);

  void ClearForces(PhysicsWorld& world);
}