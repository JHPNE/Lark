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

  private:
    SphereShape m_shape;
  };
}