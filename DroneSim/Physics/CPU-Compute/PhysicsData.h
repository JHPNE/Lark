#pragma once

#include <vector>
#include <array>
#include <cfloat>
#include <cmath>
#include <limits>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <cstdint>
#include "../../Geometry/Geometry.h"

namespace drosim::physics::cpu {

  // Memory Pool
  template<typename T>
  class MemoryPool {
    public:
      MemoryPool(size_t capacity = 1024) {
        m_data.reserve(capacity);
        m_freeList.reserve(capacity);
      }

      size_t Allocate() {
        if (!m_freeList.empty()) {
          size_t idx = m_freeList.back();
          m_freeList.pop_back();
          return idx;
        } else {
          m_data.emplace_back(T());
          return m_data.size() -1;
        }
      }

      void Free(size_t idx) {
        m_freeList.push_back(idx);
      }

      T& operator[](size_t idx) { return m_data[idx]; }
      const T& operator[](size_t idx) const { return m_data[idx]; }

      size_t Size() const { return m_data.size(); }

    private:
      std::vector<T> m_data;
      std::vector<size_t> m_freeList;
  };

  // Physics Structures
  struct BodyMotionData {
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec3 angularVelocity;
    glm::mat3 orientation;
    glm::mat3 invOrientation;
  };

  struct BodyInertiaData{
    float mass;
    float invMass;
    glm::mat3 localInertia;
    glm::mat3 invLocalInertia;
    glm::mat3 globalInvInertia;
  };

  struct BodyForceData {
    glm::vec3 force;
    glm::vec3 torque;
  };

  struct BodyMaterialData {
    float friction;
    float restitution;
  };

  struct BodyFlagsData {
    bool active;
    bool isStatic;
  };

  struct RigidBody{
    BodyMotionData motion;
    BodyInertiaData inertia;
    BodyForceData forces;
    BodyMaterialData material;
    BodyFlagsData flags;
  };

  // Colliders
  enum class ColliderType : uint8_t {
    Box = 0,
    Sphere = 1,
    ConvexMesh = 2,
  };

  struct BoxCollider {
    uint32_t bodyIndex;
    glm::vec3 localCenter;
    glm::vec3 halfExtents;
  };

  struct SphereCollider {
    uint32_t bodyIndex;
    glm::vec3 localCenter;
    float radius;
  };
  struct ConvexMeshCollider {
    uint32_t bodyIndex;
    glm::vec3 localCenter;
    drosim::tools::mesh& mesh;
  };

  // AABB Tree
  struct AABBTreeNode {
    glm::vec3 minPoint;
    glm::vec3 maxPoint;
    uint32_t parent;
    uint32_t children[2];
    bool isLeaf;
    ColliderType type;
    uint32_t colliderIndex;
  };

  struct DynamicAABBTree {
    std::vector<AABBTreeNode> nodes;
    std::vector<uint32_t> freeList;
    uint32_t root;
    float margin;

    DynamicAABBTree() : root(UINT32_MAX), margin(0.02f) {}
  };

  // Contacts and Constraints
  struct ContactPoint {
    glm::vec3 pointA;
    glm::vec3 pointB;
    glm::vec3 normal;
    float penetration;
    uint32_t bodyAIndex;
    uint32_t bodyBIndex;
  };

  struct ConstraintInfo {
    uint32_t bodyA;
    uint32_t bodyB;
    glm::vec3 localAnchorA;
    glm::vec3 localAnchorB;
    float restLength;
  };

  struct PhysicsWorld {
    // Body data
    MemoryPool<RigidBody> bodyPool;

    // Colliders
    MemoryPool<BoxCollider> boxPool;
    MemoryPool<SphereCollider> spherePool;
    // ... ConvexMeshPool if needed

    // Broad Phase
    DynamicAABBTree aabbTree;

    // Constraints (distance, hinge, etc.)
    std::vector<ConstraintInfo> constraints;

    // Contacts from narrow phase
    std::vector<ContactPoint> contacts;

    // Sleeping thresholds
    float sleepLinThreshold  = 0.05f;
    float sleepAngThreshold  = 0.05f;

    // Gravity
    // TODO lets do an ENVIRONMENT LATER
    glm::vec3 gravity = glm::vec3(0.0f, -9.81f, 0.0f);
  };
}