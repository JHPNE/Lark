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

    // Debug current positions
    const auto* bodyA = colliderA->GetRigidBody();
    const auto* bodyB = colliderB->GetRigidBody();
    std::cout << "GJK Check - BodyA pos: " << bodyA->GetPosition().x
              << " BodyB pos: " << bodyB->GetPosition().y << "\n";

    // Initial search direction (use vector between centroids)
    glm::vec3 direction = bodyB->GetPosition() - bodyA->GetPosition();
    if (glm::length2(direction) < 1e-6f) {
        direction = glm::vec3(1.0f, 0.0f, 0.0f);
    } else {
        direction = glm::normalize(direction);
    }

    // Get first support point
    auto support = ComputeSupport(colliderA, colliderB, direction);
    simplex.AddPoint(support);
    std::cout << "Initial support point: " << support.csoPoint.y << "\n";

    // New direction towards origin
    direction = -support.csoPoint;

    int iterCount = 0;
    const int MAX_ITERATIONS = 32;

    while (iterCount++ < MAX_ITERATIONS) {
        if (glm::length2(direction) < 1e-6f) {
            direction = glm::vec3(1.0f, 0.0f, 0.0f);
        } else {
            direction = glm::normalize(direction);
        }

        support = ComputeSupport(colliderA, colliderB, direction);

        // Debug support point
        std::cout << "GJK Iteration " << iterCount << " Support: " << support.csoPoint.y << "\n";

        float proj = glm::dot(support.csoPoint, direction);
        if (proj < 0.0f) {
            std::cout << "GJK: No collision (separating axis found)\n";
            return false;
        }

        simplex.AddPoint(support);

        if (simplex.DoSimpexCheck(direction)) {
            std::cout << "GJK: Collision detected! Generating contact info...\n";
            EPAAlgorithm::GenerateContact(simplex, colliderA, colliderB, contact);
            std::cout << "Contact generated - PointA: " << contact.pointA.y
                     << " PointB: " << contact.pointB.y
                     << " Normal: " << contact.normal.y
                     << " Depth: " << contact.penetrationDepth << "\n";
            return true;
        }
    }

    std::cout << "GJK: Max iterations reached\n";
    return false;
  }


  GJKSupportPoint GJKAlgorithm::ComputeSupport(const Collider* colliderA, const Collider* colliderB, const glm::vec3& direction) {
    const auto* bodyA = colliderA->GetRigidBody();
    const auto* bodyB = colliderB->GetRigidBody();

    // convert search direction to model space
    const glm::vec3 localDirA = bodyA->GlobalToLocalVec(direction);
    const glm::vec3 localDirB = bodyB->GlobalToLocalVec(direction);

    // compute support poitns
    glm::vec3 supportA = colliderA->Support(localDirA);
    glm::vec3 supportB = colliderB->Support(localDirB);

    supportA = bodyA->LocalToGlobal(supportA);
    supportB = bodyB->LocalToGlobal(supportB);

    return {
      supportA - supportB, // CSO point
      supportA, // World space point on A
      supportB
    };
  }
};