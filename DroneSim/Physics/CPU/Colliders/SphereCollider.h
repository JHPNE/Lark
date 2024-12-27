#pragma once
#include "../Collider.h"
#include "../Shapes/SphereShape.h"

namespace drosim::physics {

  class SphereCollider : public Collider {
  public:
    explicit SphereCollider(float radius)
        : Collider(SphereShape(radius)), m_shape(radius) {}

    glm::vec3 Support(const glm::vec3& direction) const override {
      return m_shape.Support(direction);
    }

    void UpdateAABBBounds() override {
      AABB* aabb = GetAABB();
      if (!aabb) return;

      RigidBody* body = GetRigidBody();
      if (!body) {
        aabb->minPoint = -m_shape.GetSize();
      }

    }

    const Shape &GetShape() const override {
      return m_shape;
    }

  private:
    SphereShape m_shape;
  };
}