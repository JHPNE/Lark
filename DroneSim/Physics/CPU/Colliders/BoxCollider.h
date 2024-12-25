#pragma once
#include "../Collider.h"
#include "../Shapes/BoxShape.h"
#include "../RigidBody.h"  // Add this forward declaration

namespace drosim::physics {
class BoxCollider : public Collider {
public:
  explicit BoxCollider(const glm::vec3& halfExtents)
      : Collider(BoxShape(halfExtents)), m_shape(halfExtents)
  {
    // Initialize AABB with box extents
    AABB* aabb = GetAABB();
    if (aabb) {
      aabb->userData = this;
      aabb->minPoint = -halfExtents;
      aabb->maxPoint =  halfExtents;
    }
  }

  void UpdateAABBBounds() {
    AABB* aabb = GetAABB();
    if (!aabb) return;

    // Get current body position and extents
    RigidBody* body = GetRigidBody();
    if (!body) {
      // If no body yet, just use local coordinates
      aabb->minPoint = -m_shape.m_halfExtents;
      aabb->maxPoint =  m_shape.m_halfExtents;
      return;
    }

    // Update AABB bounds relative to body position
    glm::vec3 pos = body->GetPosition();
    aabb->minPoint = pos - m_shape.m_halfExtents;
    aabb->maxPoint = pos + m_shape.m_halfExtents;
  }

  BoxShape m_shape;
};
}