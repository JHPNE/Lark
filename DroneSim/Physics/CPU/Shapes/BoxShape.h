#pragma once
#include "../Shape.h"

namespace drosim::physics {
  class BoxShape : public Shape {
  public:
    explicit BoxShape(const glm::vec3& halfExtents) : m_halfExtents(halfExtents) {}

    float ComputeMass() const override {
      return 8.0f * m_halfExtents.x * m_halfExtents.y * m_halfExtents.z;
    }

    glm::mat3 ComputeLocalInertiaTensor() const override {
      float x2 = m_halfExtents.x * m_halfExtents.x;
      float y2 = m_halfExtents.y * m_halfExtents.y;
      float z2 = m_halfExtents.z * m_halfExtents.z;

      return glm::mat3(
          (y2 + z2) / 3.0f, 0.0f, 0.0f,
          0.0f, (x2 + z2) / 3.0f, 0.0f,
          0.0f, 0.0f, (x2 + y2) / 3.0f
      );
    }

    glm::vec3 ComputeLocalCentroid() const override {
      return glm::vec3(0.0f);
    }

    glm::vec3 Support(const glm::vec3& direction) const {
      return glm::vec3(
          direction.x > 0.0f ? m_halfExtents.x : -m_halfExtents.x,
          direction.y > 0.0f ? m_halfExtents.y : -m_halfExtents.y,
          direction.z > 0.0f ? m_halfExtents.z : -m_halfExtents.z
      );
    }

    glm::vec3 m_halfExtents;
  private:
  };
}