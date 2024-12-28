#pragma once
#include "PhysicsData.h"

namespace drosim::physics::cpu {
  uint32_t CreateBoxCollider(PhysicsWorld& world, uint32_t bodyIndex,
                             const glm::vec3& halfExtents,
                             const glm::vec3& localCenter = glm::vec3(0.f));

  uint32_t CreateSphereCollider(PhysicsWorld& world, uint32_t bodyIndex,
                                float radius,
                                const glm::vec3& localCenter = glm::vec3(0.f));

  // Update dynamic tree
  void UpdateDynamicTree(PhysicsWorld& world);
  void BroadPhaseCollisions(PhysicsWorld& world, std::vector<std::pair<uint32_t, uint32_t>>& outPairs);

  // AABB Tree functions
  uint32_t InsertLeafNode(DynamicAABBTree& tree, const glm::vec3& minPt, const glm::vec3& maxPt,
                          ColliderType type, uint32_t colliderIndex);
  void UpdateLeafNode(DynamicAABBTree& tree, uint32_t nodeIndex,
                      const glm::vec3& minPt, const glm::vec3& maxPt);
  void RebalanceAABB(DynamicAABBTree& tree);
}