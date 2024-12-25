#pragma once
#include "../Collider.h"
#include "../Shapes/ConvexMeshShape.h"

namespace drosim::physics {
  class ConvexMeshCollider : public Collider {
  public:
    explicit ConvexMeshCollider(const std::vector<glm::vec3>& vertices)
        : Collider(ConvexMeshShape(vertices)), m_shape(vertices) {}

    glm::vec3 Support(const glm::vec3& direction) const override {
      return m_shape.Support(direction);
    }

  private:
    ConvexMeshShape m_shape;
  };
}