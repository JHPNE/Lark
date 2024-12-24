#pragma once

namespace drosim::physics {
  // Forward declaration
  class Collider;

  struct Ray3 {
    glm::vec3 pos;
    glm::vec3 dir;
  };

  struct RayCastResult {
    bool hit;
    Collider *collider;
    glm::vec3 position;
    glm::vec3 normal;
    float t;
  };
};