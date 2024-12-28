#pragma once
#include "PhysicsData.h"

namespace drosim::physics::cpu {
  // GJK/EPA structures
  struct SupportPoint {
    glm::vec3 minkowskiPoint; // Minkowski difference = supA - supB
    glm::vec3 pointA;         // support in shape A (world space)
    glm::vec3 pointB;         // support in shape B (world space)
  };

  struct Simplex {
    std::array<SupportPoint, 4> pts;
    int size = 0;
  };

  struct EPAFace {
    glm::vec3 normal;
    float distance;
    int a, b, c; // indices into polytope vertex array
  };

  // Build an initial polytope from the final GJK simplex (tetrahedron)
  struct EPAPoly {
    std::vector<SupportPoint> vertices;
    std::vector<EPAFace> faces;
  };

  // GJK/EPA
  bool GJKIntersect(const PhysicsWorld& world,
                       ColliderType typeA, uint32_t idxA,
                       ColliderType typeB, uint32_t idxB,
                       glm::vec3& outNormal,
                       float& outPenetration,
                       glm::vec3& outPointA,
                       glm::vec3& outPointB);

  bool EPACompute(PhysicsWorld& world, const Simplex& simplex,
                  uint32_t colliderA, ColliderType typeA,
                  uint32_t colliderB, ColliderType typeB,
                  ContactPoint& outContact);

  // The narrow phase function that uses GJK/EPA on broad-phase pairs
  void NarrowPhase(PhysicsWorld& world,
      const std::vector<std::pair<uint32_t, uint32_t>>& pairs);
}
