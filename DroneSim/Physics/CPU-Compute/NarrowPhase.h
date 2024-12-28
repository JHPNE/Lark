#pragma once
#include "PhysicsData.h"

namespace drosim::physics::cpu {
  // GJK/EPA structures
  struct SupportPoint {
    glm::vec3 point;   // Minkowski difference point
    glm::vec3 supA;    // Support on shape A
    glm::vec3 supB;    // Support on shape B
  };

  struct Simplex {
    std::array<SupportPoint, 4> pts;
    int size;
  };

  // GJK/EPA
  bool GJKIntersect(PhysicsWorld& world, uint32_t colliderA, ColliderType typeA,
                    uint32_t colliderB, ColliderType typeB,
                    ContactPoint& outContact);

  bool EPACompute(PhysicsWorld& world, const Simplex& simplex,
                  uint32_t colliderA, ColliderType typeA,
                  uint32_t colliderB, ColliderType typeB,
                  ContactPoint& outContact);

  // The narrow phase function that uses GJK/EPA on broad-phase pairs
  void NarrowPhase(PhysicsWorld& world,
      const std::vector<std::pair<uint32_t, uint32_t>>& pairs);
}
