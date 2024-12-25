// EPA.h
#pragma once
#include "GJK.h"
#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <vector>
#include <list>
#include <algorithm>

namespace drosim::physics {

struct EPAEdge {
    int a, b;
    EPAEdge(int a, int b) : a(a), b(b) {}
    bool operator==(const EPAEdge& other) const {
        return (a == other.a && b == other.b) || (a == other.b && b == other.a);
    }
};

struct EPAFace {
    int indices[3];           // Vertex indices
    glm::vec3 normal;         // Face normal
    float distance;           // Distance to origin
    const std::vector<GJKSupportPoint>* vertices; // Reference to vertices
    const Collider* colliderA;
    const Collider* colliderB;

    EPAFace(int a, int b, int c, const std::vector<GJKSupportPoint>& verts,
            const Collider* colA, const Collider* colB)
        : vertices(&verts)
        , colliderA(colA)
        , colliderB(colB) {
        indices[0] = a;
        indices[1] = b;
        indices[2] = c;
        ComputeNormalAndDistance();
    }

    void ComputeNormalAndDistance() {
        const auto& a = (*vertices)[indices[0]].csoPoint;
        const auto& b = (*vertices)[indices[1]].csoPoint;
        const auto& c = (*vertices)[indices[2]].csoPoint;

        glm::vec3 ab = b - a;
        glm::vec3 ac = c - a;
        normal = glm::normalize(glm::cross(ab, ac));
        distance = glm::dot(normal, a);
    }

    bool CanSeePoint(const glm::vec3& point) const {
        return glm::dot(normal, point) > distance;
    }

    glm::vec3 getBarycentricCoords() const {
        // Simplified barycentric coords
        return glm::vec3(1.0f/3.0f, 1.0f/3.0f, 1.0f/3.0f);
    }

    glm::vec3 InterpolatePoint(const glm::vec3& bary, bool isA) const {
        const auto& v0 = (*vertices)[indices[0]];
        const auto& v1 = (*vertices)[indices[1]];
        const auto& v2 = (*vertices)[indices[2]];

        if (isA) {
            return bary.x * v0.pointA + bary.y * v1.pointA + bary.z * v2.pointA;
        } else {
            return bary.x * v0.pointB + bary.y * v1.pointB + bary.z * v2.pointB;
        }
    }

    const glm::vec3& getNormal() const { return normal; }
    float getDistance() const { return distance; }
};

class EPAPolytope {
public:
    EPAPolytope(const GJKSimplex& simplex, const Collider* colA, const Collider* colB)
        : m_colliderA(colA), m_colliderB(colB) {
        InitializeFromSimplex(simplex);
    }

    const EPAFace& GetClosestFace() const;
    void ExpandWithPoint(const GJKSupportPoint& point);

private:
    void InitializeFromSimplex(const GJKSimplex& simplex);
    void CreateTetrahedralFaces();
    void EnsureCorrectWinding();
    void AddEdgeToLoop(const EPAEdge& edge, std::vector<EPAEdge>& edgeLoop);

    std::vector<GJKSupportPoint> m_vertices;
    std::list<EPAFace> m_faces;
    const Collider* m_colliderA;
    const Collider* m_colliderB;
};

class EPAAlgorithm {
public:
    static void GenerateContact(const GJKSimplex& simplex,
                              const Collider* colliderA,
                              const Collider* colliderB,
                              ContactInfo& contact);
private:
    static void GenerateContactInfo(const EPAFace& face, ContactInfo& contact);
    friend class GJKAlgorithm;
};

} // namespace drosim::physics