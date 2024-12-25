// GJK.h
#pragma once
#include "ContactInfo.h"
#include "../Collider.h"
#include <array>

namespace drosim::physics {
  struct GJKSupportPoint {
    glm::vec3 csoPoint; // Point in CSO space (A -B)
    glm::vec3 pointA;
    glm::vec3 pointB;
  };

  class GJKSimplex {
    public:
      GJKSimplex() : m_size(0) {}

      void AddPoint(const GJKSupportPoint& point);
      bool DoSimpexCheck(glm::vec3& direction);
    private:
      bool DoPointCheck(glm::vec3& direction);
      bool DoLineCheck(glm::vec3& direction);
      bool DoTriangleCheck(glm::vec3& direction);
      bool DoTetrahedronCheck(glm::vec3& direction);

      std::array<GJKSupportPoint, 4> m_points;
      int m_size;
      friend class EPAPolytope;
  };

  class GJKAlgorithm {
    public:
      static bool DetectCollision(const Collider* colliderA, const Collider* colliderB, ContactInfo& contact);
      static GJKSupportPoint ComputeSupport(const Collider* colliderA, const Collider* colliderB, const glm::vec3& direction);
  };
}