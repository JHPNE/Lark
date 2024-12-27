#include "GJK.h"
#include "EPA.h"

#include <iostream>

namespace drosim::physics {

  // GJK Simplex
  void GJKSimplex::AddPoint(const GJKSupportPoint& point) {
    if (m_size < 4) {
      m_points[m_size++] = point;
    }
  }

  bool GJKSimplex::DoSimpexCheck(glm::vec3& direction) {
    switch (m_size) {
      case 2: return DoLineCheck(direction);
      case 3: return DoTriangleCheck(direction);
      case 4: return DoTetrahedronCheck(direction);
      default: return DoPointCheck(direction);
    }
  }

  bool GJKSimplex::DoPointCheck(glm::vec3& direction) {
    direction = -m_points[0].csoPoint;
    return false;
  }

  bool GJKSimplex::DoLineCheck(glm::vec3& direction) {
    glm::vec3 a = m_points[1].csoPoint;
    glm::vec3 b = m_points[0].csoPoint;
    glm::vec3 ab = b - a;
    glm::vec3 ao = -a;

    if (glm::dot(ab, ao) > 0.0f) {
      direction = glm::cross(glm::cross(ab, ao), ab);
    } else {
      m_size = 1;
      m_points[0] = m_points[1];
      direction = ao;
    }
    return false;
  }

  bool GJKSimplex::DoTriangleCheck(glm::vec3& direction) {
    glm::vec3 a = m_points[2].csoPoint;
    glm::vec3 b = m_points[1].csoPoint;
    glm::vec3 c = m_points[0].csoPoint;

    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 ao = -a;

    glm::vec3 abc = glm::cross(ab, ac);

    if (glm::dot(glm::cross(abc, ac), ao) > 0.0f) {
        if (glm::dot(ac, ao) > 0.0f) {
            m_size = 2;
            m_points[0] = m_points[2];
            m_points[1] = m_points[0];
            direction = glm::cross(glm::cross(ac, ao), ac);
        } else {
            if (glm::dot(ab, ao) > 0.0f) {
                m_size = 2;
                m_points[0] = m_points[2];
                m_points[1] = m_points[1];
                direction = glm::cross(glm::cross(ab, ao), ab);
            } else {
                m_size = 1;
                m_points[0] = m_points[2];
                direction = ao;
            }
        }
    } else {
        if (glm::dot(glm::cross(ab, abc), ao) > 0.0f) {
            if (glm::dot(ab, ao) > 0.0f) {
                m_size = 2;
                m_points[0] = m_points[2];
                m_points[1] = m_points[1];
                direction = glm::cross(glm::cross(ab, ao), ab);
            } else {
                m_size = 1;
                m_points[0] = m_points[2];
                direction = ao;
            }
        } else {
            if (glm::dot(abc, ao) > 0.0f) {
                direction = abc;
            } else {
                GJKSupportPoint temp = m_points[0];
                m_points[0] = m_points[1];
                m_points[1] = temp;
                direction = -abc;
            }
        }
    }
    return false;
  }

  bool GJKSimplex::DoTetrahedronCheck(glm::vec3& direction) {
    glm::vec3 a = m_points[3].csoPoint;
    glm::vec3 b = m_points[2].csoPoint;
    glm::vec3 c = m_points[1].csoPoint;
    glm::vec3 d = m_points[0].csoPoint;

    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 ad = d - a;
    glm::vec3 ao = -a;

    glm::vec3 abc = glm::cross(ab, ac);
    glm::vec3 acd = glm::cross(ac, ad);
    glm::vec3 adb = glm::cross(ad, ab);

    if (glm::dot(abc, ao) > 0.0f) {
      m_size = 3;
      m_points[0] = m_points[1];
      m_points[1] = m_points[2];
      m_points[2] = m_points[3];
      return DoTriangleCheck(direction);
    }

    if (glm::dot(acd, ao) > 0.0f) {
      m_size = 3;
      m_points[0] = m_points[3];
      return DoTriangleCheck(direction);
    }

    if (glm::dot(adb, ao) > 0.0f) {
      m_size = 3;
      m_points[0] = m_points[2];
      m_points[2] = m_points[3];
      return DoTriangleCheck(direction);
    }

    return true; // Origin is inside tetrahedron
  }

  // GJK
  bool GJKAlgorithm::DetectCollision(const Collider* colliderA, const Collider* colliderB, ContactInfo& contact) {
    GJKSimplex simplex;

    const auto* bodyA = colliderA->GetRigidBody();
    const auto* bodyB = colliderB->GetRigidBody();

    // Initial direction from center of A to center of B
    glm::vec3 direction = bodyB->GetPosition() - bodyA->GetPosition();

    // If centers are too close, use a fallback direction
    if (glm::length2(direction) < 1e-6f) {
        direction = glm::vec3(0.0f, 1.0f, 0.0f);
    }
    direction = glm::normalize(direction);

    // Get first support point
    auto support = ComputeSupport(colliderA, colliderB, direction);
    simplex.AddPoint(support);

    // New direction towards origin
    direction = -support.csoPoint;

    int iterCount = 0;
    const int MAX_ITERATIONS = 32;

    while (iterCount++ < MAX_ITERATIONS) {
        float dirLength = glm::length(direction);
        if (dirLength < 1e-6f) {
            // If direction is zero, we're likely at the origin
            // This usually means we have a collision
            EPAAlgorithm::GenerateContact(simplex, colliderA, colliderB, contact);
            return true;
        }

        direction /= dirLength;  // Normalize

        support = ComputeSupport(colliderA, colliderB, direction);

        float proj = glm::dot(support.csoPoint, direction);
        if (proj <= 0.0f) {
            // Found separating axis
            return false;
        }

        simplex.AddPoint(support);

        if (simplex.DoSimpexCheck(direction)) {
            // Found intersection
            EPAAlgorithm::GenerateContact(simplex, colliderA, colliderB, contact);
            return true;
        }
    }

    // If we reach here, consider it a collision but warn about max iterations
    std::cout << "Warning: GJK reached maximum iterations\n";
    EPAAlgorithm::GenerateContact(simplex, colliderA, colliderB, contact);
    return true;
  }


  GJKSupportPoint GJKAlgorithm::ComputeSupport(const Collider* colliderA, const Collider* colliderB, const glm::vec3& direction) {
    const auto* bodyA = colliderA->GetRigidBody();
    const auto* bodyB = colliderB->GetRigidBody();

    // Transform direction to local space of each body
    glm::vec3 localDirA = bodyA->GlobalToLocalVec(direction);
    glm::vec3 localDirB = bodyB->GlobalToLocalVec(-direction);  // Note the negation for B

    // Get support points in local space
    glm::vec3 supportA = colliderA->Support(localDirA);
    glm::vec3 supportB = colliderB->Support(localDirB);

    // Transform support points to world space
    glm::vec3 worldA = bodyA->LocalToGlobal(supportA);
    glm::vec3 worldB = bodyB->LocalToGlobal(supportB);

    // Return the Minkowski difference point
    return {
      worldA - worldB,  // CSO point
      worldA,           // World space point on A
      worldB            // World space point on B
    };
  }
};