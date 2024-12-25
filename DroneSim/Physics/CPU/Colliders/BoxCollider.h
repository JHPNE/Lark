#pragma once
#include "../Collider.h"
#include "../Shapes/BoxShape.h"

namespace drosim::physics {

  class BoxCollider : public Collider {
  public:
    explicit BoxCollider(const glm::vec3& halfExtents)
        : Collider(BoxShape(halfExtents)), m_shape(halfExtents) {

      // Initialize AABB with box extents
      AABB* aabb = GetAABB();
      if (aabb) {
        // Just set to half extents initially - will be updated when added to body
        aabb->minPoint = -halfExtents;
        aabb->maxPoint = halfExtents;
        aabb->userData = this;  // Set collider as user data
      }
    }

    glm::vec3 Support(const glm::vec3& direction) const override {
      return m_shape.Support(direction);
    }

    BoxShape m_shape;
  private:
  };
}