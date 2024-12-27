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

      glm::vec3 Support(const glm::vec3& direction) const;

      glm::vec3 GetSize() const override {
        return glm::vec3(1.0f);
      }

    private:
      std::vector<glm::vec3> m_vertices;
      // Possibly store volume, etc., once computed
  };
}