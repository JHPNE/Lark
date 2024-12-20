#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>
#include <vector>

struct Environment {
  glm::vec3 Gravity = glm::vec3(0.0f, -9.81f, 0.0f);
};

struct RigidBodyArrays {
  std::vector<glm::vec3> positions;
  std::vector<glm::quat> orientations;
  std::vector<glm::vec3> linearVelocities;
  std::vector<glm::vec3> angularVelocities;

  // For mass/inertia:
  // mass and invMass are scalars
  // Inertia and invInertia are diagonal vectors (ix, iy, iz)
  // We'll store them in two arrays of vec4 because the shader expects vec4:
  // Layout:
  // massData[i]   = (mass, invMass, inertia.x, inertia.y)
  // inertiaData[i]= (inertia.z, invInertia.x, invInertia.y, invInertia.z)
  std::vector<glm::vec4> massData;
  std::vector<glm::vec4> inertiaData;
};

// Axis Aligned Bounding Box
struct AABB {
  glm::vec3 min;
  glm::vec3 max;

  bool overlaps(const AABB& other) const {
    return !(max.x < other.min.x || min.x > other.max.x ||
             max.y < other.min.y || min.y > other.max.y ||
             max.z < other.min.z || min.z > other.max.z);
  }

  void expand(float amount) {
    min.x -= amount; min.y -= amount; min.z -= amount;
    max.x += amount; max.y += amount; max.z += amount;
  }
};

inline AABB expandAABB(const AABB& a, const AABB& b) {
  AABB result;
  result.min = glm::min(a.min, b.min);
  result.max = glm::max(a.max, b.max);
  return result;
}

inline float surfaceArea(const AABB& aabb) {
  glm::vec3 d = aabb.max - aabb.min;
  return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
}