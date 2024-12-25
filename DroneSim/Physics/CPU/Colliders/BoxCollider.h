#pragma once
#include "../Collider.h"
#include "../Shapes/BoxShape.h"

namespace drosim::physics {

  class BoxCollider : public Collider {
  public:
    explicit BoxCollider(const glm::vec3& halfExtents)
        : Collider(BoxShape(halfExtents)), m_shape(halfExtents) {}

    glm::vec3 Support(const glm::vec3& direction) const override {
      return m_shape.Support(direction);
    }

  private:
    BoxShape m_shape;
  };
}