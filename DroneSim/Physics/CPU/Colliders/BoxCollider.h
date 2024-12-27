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

    RigidBody* body = GetRigidBody();
    if (!body) {
      aabb->minPoint = -m_shape.m_halfExtents;
      aabb->maxPoint = m_shape.m_halfExtents;
      return;
    }

    // Transform the box corners by the body's orientation
    glm::mat3 orientation = body->GetOrientation();
    glm::vec3 pos = body->GetPosition();

    // Compute oriented extents
    glm::vec3 orientedExtents;
    for (int i = 0; i < 3; ++i) {
      orientedExtents[i] =
          std::abs(orientation[0][i] * m_shape.m_halfExtents.x) +
          std::abs(orientation[1][i] * m_shape.m_halfExtents.y) +
          std::abs(orientation[2][i] * m_shape.m_halfExtents.z);
    }

    aabb->minPoint = pos - orientedExtents;
    aabb->maxPoint = pos + orientedExtents;
  }

  glm::vec3 Support(const glm::vec3& direction) const override {
    // For a box, return the vertex most extreme in the given direction
    return glm::vec3(
        direction.x > 0.0f ? m_shape.m_halfExtents.x : -m_shape.m_halfExtents.x,
        direction.y > 0.0f ? m_shape.m_halfExtents.y : -m_shape.m_halfExtents.y,
        direction.z > 0.0f ? m_shape.m_halfExtents.z : -m_shape.m_halfExtents.z
    );
  }

  BoxShape m_shape;
};
}