#pragma once

#include "../Shape.h"
#include <vector>

namespace drosim::physics {
  class ConvexMeshShape : public Shape {
    public:
      explicit ConvexMeshShape(const std::vector<glm::vec3> &vertices);

      float ComputeMass() const override;
      glm::mat3 ComputeLocalInertiaTensor() const override;
      glm::vec3 ComputeLocalCentroid() const override;

    private:
      std::vector<glm::vec3> m_vertices;
      // Possibly store volume, etc., once computed
  };
}