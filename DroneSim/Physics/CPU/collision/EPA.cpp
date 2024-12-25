#include "EPA.h"
#include <glm/geometric.hpp>
#include "../RigidBody.h"

namespace drosim::physics {

const EPAFace& EPAPolytope::GetClosestFace() const {
    auto it = std::min_element(m_faces.begin(), m_faces.end(),
        [](const EPAFace& a, const EPAFace& b) {
            return std::abs(a.distance) < std::abs(b.distance);
        });
    return *it;
}

void EPAPolytope::ExpandWithPoint(const GJKSupportPoint& point) {
    std::vector<EPAEdge> edgeLoop;
    int newIndex = static_cast<int>(m_vertices.size());
    m_vertices.push_back(point);

    // Find all faces that can see the point
    auto it = m_faces.begin();
    while (it != m_faces.end()) {
        if (it->CanSeePoint(point.csoPoint)) {
            // Add edges of the face to edge loop
            AddEdgeToLoop(EPAEdge(it->indices[0], it->indices[1]), edgeLoop);
            AddEdgeToLoop(EPAEdge(it->indices[1], it->indices[2]), edgeLoop);
            AddEdgeToLoop(EPAEdge(it->indices[2], it->indices[0]), edgeLoop);

            // Remove face
            it = m_faces.erase(it);
        } else {
            ++it;
        }
    }

    // Create new faces using edge loop
    for (const auto& edge : edgeLoop) {
        m_faces.emplace_back(edge.a, edge.b, newIndex, m_vertices, m_colliderA, m_colliderB);
    }
}

void EPAPolytope::InitializeFromSimplex(const GJKSimplex& simplex) {
    m_vertices.clear();
    for (int i = 0; i < simplex.m_size; ++i) {
        m_vertices.push_back(simplex.m_points[i]);
    }

    if (m_vertices.size() >= 4) {
        CreateTetrahedralFaces();
    }
}

void EPAPolytope::CreateTetrahedralFaces() {
    m_faces.clear();
    m_faces.emplace_back(0, 1, 2, m_vertices, m_colliderA, m_colliderB);
    m_faces.emplace_back(0, 2, 3, m_vertices, m_colliderA, m_colliderB);
    m_faces.emplace_back(0, 3, 1, m_vertices, m_colliderA, m_colliderB);
    m_faces.emplace_back(1, 3, 2, m_vertices, m_colliderA, m_colliderB);

    EnsureCorrectWinding();
}

void EPAPolytope::EnsureCorrectWinding() {
    for (auto& face : m_faces) {
        const auto& a = m_vertices[face.indices[0]].csoPoint;
        glm::vec3 center = a + m_vertices[face.indices[1]].csoPoint + m_vertices[face.indices[2]].csoPoint;
        center /= 3.0f;

        if (glm::dot(face.normal, center) < 0.0f) {
            std::swap(face.indices[1], face.indices[2]);
            face.ComputeNormalAndDistance();
        }
    }
}

void EPAPolytope::AddEdgeToLoop(const EPAEdge& edge, std::vector<EPAEdge>& edgeLoop) {
    auto it = std::find(edgeLoop.begin(), edgeLoop.end(), edge);
    if (it != edgeLoop.end()) {
        edgeLoop.erase(it);
    } else {
        edgeLoop.push_back(edge);
    }
}

void EPAAlgorithm::GenerateContact(const GJKSimplex& simplex,
                                 const Collider* colliderA,
                                 const Collider* colliderB,
                                 ContactInfo& contact) {
    EPAPolytope polytope(simplex, colliderA, colliderB);
    const float TOLERANCE = 0.0001f;

    while (true) {
        const auto& face = polytope.GetClosestFace();
        glm::vec3 normal = face.getNormal();

        auto support = GJKAlgorithm::ComputeSupport(colliderA, colliderB, normal);
        float dist = glm::dot(support.csoPoint, normal);

        if (dist - face.getDistance() < TOLERANCE) {
            GenerateContactInfo(face, contact);
            return;
        }

        polytope.ExpandWithPoint(support);
    }
}

void EPAAlgorithm::GenerateContactInfo(const EPAFace& face, ContactInfo& contact) {
    glm::vec3 normal = face.getNormal();
    float depth = face.getDistance();

    glm::vec3 bary = face.getBarycentricCoords();

    contact.normal = normal;
    contact.penetrationDepth = depth;

    contact.pointA = face.InterpolatePoint(bary, true);
    contact.pointB = face.InterpolatePoint(bary, false);

    // Convert to local space
    const auto* bodyA = face.colliderA->GetRigidBody();
    const auto* bodyB = face.colliderB->GetRigidBody();

    contact.bodyA = face.colliderA->GetRigidBody();
    contact.bodyB = face.colliderB->GetRigidBody();

    if (bodyA && bodyB) {
        contact.localA = bodyA->GlobalToLocal(contact.pointA);
        contact.localB = bodyB->GlobalToLocal(contact.pointB);
    } else {
        contact.localA = contact.pointA;
        contact.localB = contact.pointB;
    }

    // Compute orthonormal basis
    if (std::abs(normal.x) >= 0.57735f) {
        contact.tangent1 = glm::normalize(glm::vec3(normal.y, -normal.x, 0.0f));
    } else {
        contact.tangent1 = glm::normalize(glm::vec3(0.0f, normal.z, -normal.y));
    }

    contact.tangent2 = glm::cross(normal, contact.tangent1);
}

} // namespace drosim::physics