#pragma once
#include "../Shape.h"

namespace drosim::physics {
  class SphereShape : public Shape {
  public:
    explicit SphereShape(float radius) : m_radius(radius) {}

    float ComputeMass() const override {
      return (4.0f/3.0f) * glm::pi<float>() * m_radius * m_radius * m_radius;
    }

    glm::mat3 ComputeLocalInertiaTensor() const override {
      float diagonal = 0.4f * m_radius * m_radius;
      return glm::mat3(
          diagonal, 0.0f, 0.0f,
          0.0f, diagonal, 0.0f,
          0.0f, 0.0f, diagonal
      );
    }

    glm::vec3 ComputeLocalCentroid() const override {
      return glm::vec3(0.0f);
    }

    glm::vec3 Support(const glm::vec3& direction) const {
      return glm::normalize(direction) * m_radius;
    }

    glm::vec3 GetSize() const override {
      return glm::vec3(m_radius);
    }

  private:
    float m_radius;
  };
}