#pragma once
#include <glm/glm.hpp>
#include "../RigidBody.h"

namespace drosim::physics {
  struct ContactInfo {
    glm::vec3 pointA;           // Contact point on first collider (world space)
    glm::vec3 pointB;           // Contact point on second collider (world space)
    glm::vec3 localA;           // Contact point on first collider (local space)
    glm::vec3 localB;           // Contact point on second collider (local space)
    glm::vec3 normal;           // Contact normal (pointing from B to A)
    glm::vec3 tangent1;         // First tangent vector
    glm::vec3 tangent2;         // Second tangent vector
    float penetrationDepth;     // Penetration depth

    RigidBody* bodyA = nullptr;
    RigidBody* bodyB = nullptr;
  };
}