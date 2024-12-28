// NarrowPhase.h
#pragma once
#include "PhysicsData.h"
#include <glm/glm.hpp>
#include <array>

namespace drosim::physics::cpu {

struct SupportPoint {
    glm::vec3 csoPoint;      // Point in CSO space (A - B)
    glm::vec3 pointA;        // World space point on A
    glm::vec3 pointB;        // World space point on B
};

struct Simplex {
    std::array<SupportPoint, 4> points;
    int size;

    Simplex() : size(0) {}
};

// EPA structures optimized for DOD
struct EPAEdge {
    uint32_t a;
    uint32_t b;
    
    bool operator==(const EPAEdge& other) const {
        return (a == other.a && b == other.b) || (a == other.b && b == other.a);
    }
};

struct EPAFace {
    uint32_t indices[3];     // Vertex indices
    glm::vec3 normal;        // Face normal
    float distance;          // Distance to origin
};

struct EPAPolytopeData {
    static constexpr size_t MAX_VERTICES = 64;
    static constexpr size_t MAX_FACES = 128;
    
    std::array<SupportPoint, MAX_VERTICES> vertices;
    std::array<EPAFace, MAX_FACES> faces;
    uint32_t vertexCount;
    uint32_t faceCount;
    
    EPAPolytopeData() : vertexCount(0), faceCount(0) {}
};

// Core GJK/EPA functions
bool GJKIntersect(const PhysicsWorld& world,
                 ColliderType typeA, uint32_t idxA,
                 ColliderType typeB, uint32_t idxB,
                 glm::vec3& outNormal,
                 float& outPenetration,
                 glm::vec3& outPointA,
                 glm::vec3& outPointB);

// Process contact detection for multiple collision pairs
void NarrowPhase(PhysicsWorld& world,
                 const std::vector<std::pair<uint32_t, uint32_t>>& pairs);

} // namespace drosim::physics::cpu