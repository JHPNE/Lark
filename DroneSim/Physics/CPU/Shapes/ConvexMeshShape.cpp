#include "ConvexMeshShape.h"
#include <algorithm>
#include <numeric>

namespace drosim::physics {
  ConvexMeshShape::ConvexMeshShape(const std::vector<glm::vec3>& vertices)
    : m_vertices(vertices) {

    std::sort(m_vertices.begin(), m_vertices.end(),
      [](const glm::vec3& a, const glm::vec3& b) {
        if (a.x != b.x) return a.x < b.x;
        if (a.y != b.y) return a.y < b.y;
        return a.z < b.z;
    });

    m_vertices.erase(
        std::unique(m_vertices.begin(), m_vertices.end(),
            [](const glm::vec3& a, const glm::vec3& b) {
                return glm::length(a - b) < 1e-7f;
            }),
        m_vertices.end());
  }

  float ConvexMeshShape::ComputeMass() const {
    float volume = 0.0f;
    glm::vec3 centroid(0.0f);

    for (const auto& v: m_vertices) {
      volume += glm::dot(v, v);
      centroid += v;
    }

    volume /= 6.0f;
    return std::abs(volume);
  }

glm::mat3 ConvexMeshShape::ComputeLocalInertiaTensor() const {
    glm::mat3 inertia(0.0f);

    // Compute inertia tensor using parallel axis theorem
    for (const auto& v : m_vertices) {
      float x2 = v.x * v.x;
      float y2 = v.y * v.y;
      float z2 = v.z * v.z;

      inertia[0][0] += y2 + z2;
      inertia[1][1] += x2 + z2;
      inertia[2][2] += x2 + y2;

      inertia[0][1] -= v.x * v.y;
      inertia[0][2] -= v.x * v.z;
      inertia[1][2] -= v.y * v.z;
    }

    // Mirror the symmetric components
    inertia[1][0] = inertia[0][1];
    inertia[2][0] = inertia[0][2];
    inertia[2][1] = inertia[1][2];

    return inertia / 12.0f;  // Scale factor for solid convex mesh
  }

  glm::vec3 ConvexMeshShape::ComputeLocalCentroid() const {
    return std::accumulate(m_vertices.begin(), m_vertices.end(), glm::vec3(0.0f))
           / static_cast<float>(m_vertices.size());
  }

  glm::vec3 ConvexMeshShape::Support(const glm::vec3& direction) const {
    // Hill climbing optimization for support function
    // Start with any vertex (middle one is usually a good start)
    size_t bestIdx = m_vertices.size() / 2;
    float bestDot = glm::dot(m_vertices[bestIdx], direction);

    bool improved;
    do {
      improved = false;

      // Check neighboring vertices
      for (size_t i = 0; i < m_vertices.size(); ++i) {
        float dot = glm::dot(m_vertices[i], direction);
        if (dot > bestDot) {
          bestDot = dot;
          bestIdx = i;
          improved = true;
          break;  // Found a better vertex, restart search from here
        }
      }
    } while (improved);

    return m_vertices[bestIdx];
  }

}