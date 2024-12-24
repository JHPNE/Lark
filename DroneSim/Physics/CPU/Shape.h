#pragma once
#include <glm/glm.hpp>

namespace drosim::physics {
  class Shape {
  public:
    virtual ~Shape() = default;

    // Each shape must implement these:
    virtual float ComputeMass() const = 0;
    virtual glm::mat3 ComputeLocalInertiaTensor() const = 0;
    virtual glm::vec3 ComputeLocalCentroid() const = 0;
  };
}