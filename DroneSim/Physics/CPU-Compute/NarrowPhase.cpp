#include "NarrowPhase.h"
#include "ColliderSystem.h" // might need for collider data

#include <iomanip>
#include <iostream>

namespace drosim::physics::cpu {

    namespace {
        uint32_t GetBodyIndex(const PhysicsWorld& world, ColliderType type, uint32_t colliderIndex) {
            return (type == ColliderType::Box)
                   ? world.boxPool[colliderIndex].bodyIndex
                   : world.spherePool[colliderIndex].bodyIndex;
        }

        void AddEdgeToLoop(const EPAEdge& edge, std::vector<EPAEdge>& edgeLoop) {
            auto it = std::find(edgeLoop.begin(), edgeLoop.end(), edge);
            if (it != edgeLoop.end()) {
                edgeLoop.erase(it);
            } else {
                edgeLoop.push_back(edge);
            }
        }

        glm::vec3 ComputeBarycentric(const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3) {
            glm::vec3 normal = glm::normalize(glm::cross(p2 - p1, p3 - p1));

            // Areas of triangles using normal and cross product
            float areaABC = glm::dot(normal, glm::cross(p2 - p1, p3 - p1));
            float areaABP = glm::dot(normal, glm::cross(p2 - p1, -p1));
            float areaBCP = glm::dot(normal, glm::cross(p3 - p2, -p2));

            float u = areaABP / areaABC;
            float v = areaBCP / areaABC;
            float w = 1.0f - u - v;

            return glm::vec3(w, u, v);
        }

        glm::vec3 GetBoxSupport(const PhysicsWorld& world, uint32_t colliderIndex, const glm::vec3& dirWorld) {
            const auto& box = world.boxPool[colliderIndex];
            const auto& body = world.bodyPool[box.bodyIndex];

            glm::mat3 rot = glm::mat3_cast(body.motion.orientation);
            glm::vec3 localDir = glm::transpose(rot) * dirWorld;

            glm::vec3 localSupport = box.localCenter + glm::vec3(
                localDir.x >= 0.0f ? box.halfExtents.x : -box.halfExtents.x,
                localDir.y >= 0.0f ? box.halfExtents.y : -box.halfExtents.y,
                localDir.z >= 0.0f ? box.halfExtents.z : -box.halfExtents.z
            );

            return body.motion.position + (rot * localSupport);
        }

        glm::vec3 GetSphereSupport(const PhysicsWorld& world, uint32_t colliderIndex, const glm::vec3& dirWorld) {
            const auto& sphere = world.spherePool[colliderIndex];
            const auto& body = world.bodyPool[sphere.bodyIndex];

            glm::mat3 rot = glm::mat3_cast(body.motion.orientation);
            float len = glm::length(dirWorld);
            if (len < 1e-6f) return body.motion.position + rot * sphere.localCenter;

            glm::vec3 normalized = dirWorld / len;
            return body.motion.position + rot * (sphere.localCenter + normalized * sphere.radius);
        }

        static glm::vec3 GetColliderSupport(const PhysicsWorld& world,
                                        ColliderType type,
                                        uint32_t colliderIndex,
                                        const glm::vec3& dirWorld)
        {
            switch (type) {
            case ColliderType::Box:
                return GetBoxSupport(world, colliderIndex, dirWorld);
            case ColliderType::Sphere:
                return GetSphereSupport(world, colliderIndex, dirWorld);
                // case ColliderType::ConvexMesh:
                //   ... implement similarly with hill-climbing or scanning all vertices ...
            default:
                return glm::vec3(0.f);
            }
        }

        SupportPoint ComputeDirectSupport(const PhysicsWorld& world,
                       ColliderType typeA, uint32_t idxA,
                       ColliderType typeB, uint32_t idxB,
                       const glm::vec3& direction) {
            glm::vec3 pA = (typeA == ColliderType::Box) ?
                GetBoxSupport(world, idxA, direction) :
                GetSphereSupport(world, idxA, direction);

            glm::vec3 pB = (typeB == ColliderType::Box) ?
                GetBoxSupport(world, idxB, -direction) :
                GetSphereSupport(world, idxB, -direction);

            return {pA - pB, pA, pB};
        }

        // Simplex helper functions:
        bool DoSimplex1(const Simplex& simplex, glm::vec3& direction) {
            direction = -simplex.points[0].csoPoint;
            return false;
        }

        bool DoSimplex2(Simplex& simplex, glm::vec3& direction) {
            const glm::vec3& a = simplex.points[1].csoPoint;
            const glm::vec3& b = simplex.points[0].csoPoint;
            const glm::vec3 ab = b - a;
            const glm::vec3 ao = -a;

            if (glm::dot(ab, ao) > 0.0f) {
                direction = glm::cross(glm::cross(ab, ao), ab);
                if (glm::length2(direction) < 1e-6f) {
                    direction = glm::cross(ab, glm::vec3(0, 1, 0));
                    if (glm::length2(direction) < 1e-6f)
                        direction = glm::cross(ab, glm::vec3(1, 0, 0));
                    direction = glm::normalize(direction);
                }
            } else {
                simplex.points[0] = simplex.points[1];
                simplex.size = 1;
                direction = ao;
            }
            return false;
        }

        bool DoSimplex3(Simplex& simplex, glm::vec3& direction) {
            const glm::vec3& a = simplex.points[2].csoPoint;
            const glm::vec3& b = simplex.points[1].csoPoint;
            const glm::vec3& c = simplex.points[0].csoPoint;

            const glm::vec3 ab = b - a;
            const glm::vec3 ac = c - a;
            const glm::vec3 abc = glm::cross(ab, ac);
            const glm::vec3 ao = -a;

            const glm::vec3 abPerp = glm::cross(abc, ab);
            if (glm::dot(abPerp, ao) > 0.0f) {
                simplex.size = 2;
                simplex.points[0] = simplex.points[1];
                simplex.points[1] = simplex.points[2];
                direction = glm::cross(glm::cross(ab, ao), ab);
                return false;
            }

            const glm::vec3 acPerp = glm::cross(ac, abc);
            if (glm::dot(acPerp, ao) > 0.0f) {
                simplex.size = 2;
                simplex.points[0] = simplex.points[2];
                direction = glm::cross(glm::cross(ac, ao), ac);
                return false;
            }

            if (glm::dot(abc, ao) > 0.0f) {
                direction = abc;
            } else {
                SupportPoint temp = simplex.points[0];
                simplex.points[0] = simplex.points[1];
                simplex.points[1] = temp;
                direction = -abc;
            }
            return false;
        }

        bool DoSimplex4(Simplex& simplex, glm::vec3& direction) {
            const glm::vec3& a = simplex.points[3].csoPoint;
            const glm::vec3& b = simplex.points[2].csoPoint;
            const glm::vec3& c = simplex.points[1].csoPoint;
            const glm::vec3& d = simplex.points[0].csoPoint;

            const glm::vec3 ab = b - a;
            const glm::vec3 ac = c - a;
            const glm::vec3 ad = d - a;
            const glm::vec3 ao = -a;

            const glm::vec3 abc = glm::cross(ab, ac);
            const glm::vec3 acd = glm::cross(ac, ad);
            const glm::vec3 adb = glm::cross(ad, ab);

            if (glm::dot(abc, ao) > 0.0f) {
                simplex.size = 3;
                simplex.points[0] = simplex.points[1];
                simplex.points[1] = simplex.points[2];
                simplex.points[2] = simplex.points[3];
                return DoSimplex3(simplex, direction);
            }

            if (glm::dot(acd, ao) > 0.0f) {
                simplex.size = 3;
                simplex.points[0] = simplex.points[3];
                return DoSimplex3(simplex, direction);
            }

            if (glm::dot(adb, ao) > 0.0f) {
                simplex.size = 3;
                simplex.points[0] = simplex.points[2];
                simplex.points[2] = simplex.points[3];
                return DoSimplex3(simplex, direction);
            }

            return true;
        }

        bool UpdateSimplex(Simplex& simplex, glm::vec3& direction) {
            switch (simplex.size) {
            case 1: return DoSimplex1(simplex, direction);
            case 2: return DoSimplex2(simplex, direction);
            case 3: return DoSimplex3(simplex, direction);
            case 4: return DoSimplex4(simplex, direction);
            default: return false;
            }
        }

        void BuildInitialEPAPolytope(const Simplex& simplex, EPAPolytopeData& polytope) {
            // Copy vertices from simplex
            polytope.vertexCount = simplex.size;
            for (int i = 0; i < simplex.size; ++i) {
                polytope.vertices[i] = simplex.points[i];
            }

            // Create initial faces
            static const uint32_t tetraIndices[4][3] = {
                {0,1,2}, {0,2,3}, {0,3,1}, {1,3,2}
            };

            polytope.faceCount = 0;
            for (int i = 0; i < 4; ++i) {
                EPAFace& face = polytope.faces[polytope.faceCount];
                face.indices[0] = tetraIndices[i][0];
                face.indices[1] = tetraIndices[i][1];
                face.indices[2] = tetraIndices[i][2];

                const glm::vec3& a = polytope.vertices[face.indices[0]].csoPoint;
                const glm::vec3& b = polytope.vertices[face.indices[1]].csoPoint;
                const glm::vec3& c = polytope.vertices[face.indices[2]].csoPoint;

                face.normal = glm::normalize(glm::cross(b - a, c - a));
                face.distance = glm::dot(face.normal, a);

                if (face.distance < 0.0f) {
                    face.normal = -face.normal;
                    face.distance = -face.distance;
                    std::swap(face.indices[1], face.indices[2]);
                }

                polytope.faceCount++;
            }
        }

        bool RunEPAAlgorithm(const PhysicsWorld& world,
            const Simplex& simplex,
            ColliderType typeA, uint32_t idxA,
            ColliderType typeB, uint32_t idxB,
            glm::vec3& outNormal,
            float& outPenetration,
            glm::vec3& outPointA,
            glm::vec3& outPointB) {
            EPAPolytopeData polytope;
            BuildInitialEPAPolytope(simplex, polytope);

            const float TOLERANCE = 0.0001f;
            const int MAX_ITERATIONS = 64;

            for (int iteration = 0; iteration < MAX_ITERATIONS; ++iteration) {
                // Find closest face to origin
                float minDist = FLT_MAX;
                int closestFace = -1;

                for (uint32_t i = 0; i < polytope.faceCount; ++i) {
                    if (polytope.faces[i].distance < minDist) {
                        minDist = polytope.faces[i].distance;
                        closestFace = i;
                    }
                }

                if (closestFace < 0) return false;

                const EPAFace& face = polytope.faces[closestFace];
                SupportPoint support = ComputeDirectSupport(world, typeA, idxA, typeB, idxB, face.normal);

                float supportDist = glm::dot(support.csoPoint, face.normal);

                // Check if we've reached the edge of the Minkowski sum
                if (std::abs(supportDist - minDist) < TOLERANCE) {
                    outNormal = face.normal;
                    outPenetration = minDist;

                    // Compute closest points
                    glm::vec3 barycentric = ComputeBarycentric(polytope.vertices[face.indices[0]].csoPoint,
                                                              polytope.vertices[face.indices[1]].csoPoint,
                                                              polytope.vertices[face.indices[2]].csoPoint);

                    outPointA = barycentric.x * polytope.vertices[face.indices[0]].pointA +
                               barycentric.y * polytope.vertices[face.indices[1]].pointA +
                               barycentric.z * polytope.vertices[face.indices[2]].pointA;

                    outPointB = barycentric.x * polytope.vertices[face.indices[0]].pointB +
                               barycentric.y * polytope.vertices[face.indices[1]].pointB +
                               barycentric.z * polytope.vertices[face.indices[2]].pointB;

                    return true;
                }

                // Otherwise expand polytope
                std::vector<EPAEdge> edgeLoop;

                // Remove faces that can see the new point
                for (uint32_t i = 0; i < polytope.faceCount;) {
                    const EPAFace& checkFace = polytope.faces[i];
                    if (glm::dot(support.csoPoint - polytope.vertices[checkFace.indices[0]].csoPoint,
                                checkFace.normal) > 0.0f) {
                        // Face can see point - add edges to loop and remove face
                        AddEdgeToLoop(EPAEdge{checkFace.indices[0], checkFace.indices[1]}, edgeLoop);
                        AddEdgeToLoop(EPAEdge{checkFace.indices[1], checkFace.indices[2]}, edgeLoop);
                        AddEdgeToLoop(EPAEdge{checkFace.indices[2], checkFace.indices[0]}, edgeLoop);

                        polytope.faces[i] = polytope.faces[polytope.faceCount - 1];
                        polytope.faceCount--;
                                } else {
                                    i++;
                                }
                }

                // Add new vertex
                if (polytope.vertexCount >= EPAPolytopeData::MAX_VERTICES) {
                    // If we run out of space, just return current best result
                    outNormal = face.normal;
                    outPenetration = minDist;
                    return true;
                }

                uint32_t newVertexIdx = polytope.vertexCount++;
                polytope.vertices[newVertexIdx] = support;

                // Create new faces using edge loop
                for (const EPAEdge& edge : edgeLoop) {
                    if (polytope.faceCount >= EPAPolytopeData::MAX_FACES) {
                        outNormal = face.normal;
                        outPenetration = minDist;
                        return true;
                    }

                    EPAFace& newFace = polytope.faces[polytope.faceCount++];
                    newFace.indices[0] = edge.a;
                    newFace.indices[1] = edge.b;
                    newFace.indices[2] = newVertexIdx;

                    const glm::vec3& a = polytope.vertices[newFace.indices[0]].csoPoint;
                    const glm::vec3& b = polytope.vertices[newFace.indices[1]].csoPoint;
                    const glm::vec3& c = polytope.vertices[newFace.indices[2]].csoPoint;

                    newFace.normal = glm::normalize(glm::cross(b - a, c - a));
                    newFace.distance = glm::dot(newFace.normal, a);

                    if (newFace.distance < 0.0f) {
                        newFace.normal = -newFace.normal;
                        newFace.distance = -newFace.distance;
                        std::swap(newFace.indices[1], newFace.indices[2]);
                    }
                }
            }

            // If we reach max iterations, return best result found
            const EPAFace& bestFace = *std::min_element(polytope.faces.begin(),
                                                       polytope.faces.begin() + polytope.faceCount,
                                                       [](const EPAFace& a, const EPAFace& b) {
                                                           return a.distance < b.distance;
                                                       });

            outNormal = bestFace.normal;
            outPenetration = bestFace.distance;
            return true;
        }
    }

    bool GJKIntersect(const PhysicsWorld& world,
                  ColliderType typeA, uint32_t idxA,
                  ColliderType typeB, uint32_t idxB,
                  glm::vec3& outNormal,
                  float& outPenetration,
                  glm::vec3& outPointA,
                  glm::vec3& outPointB) {

        Simplex simplex;

        // Get initial search direction from centers
        glm::vec3 centerA = world.bodyPool[GetBodyIndex(world, typeA, idxA)].motion.position;
        glm::vec3 centerB = world.bodyPool[GetBodyIndex(world, typeB, idxB)].motion.position;
        glm::vec3 direction = centerB - centerA;

        if (glm::length2(direction) < 1e-6f) {
            direction = glm::vec3(0.0f, 1.0f, 0.0f);
        }
        direction = glm::normalize(direction);

        // Get first support point
        simplex.points[0] = ComputeDirectSupport(world, typeA, idxA, typeB, idxB, direction);
        simplex.size = 1;

        // New direction is towards origin
        direction = -simplex.points[0].csoPoint;

        const int MAX_ITERATIONS = 32;
        for (int iteration = 0; iteration < MAX_ITERATIONS; ++iteration) {
            float dirLength = glm::length(direction);
            if (dirLength < 1e-6f) {
                return RunEPAAlgorithm(world, simplex, typeA, idxA, typeB, idxB,
                          outNormal, outPenetration, outPointA, outPointB);
            }

            direction /= dirLength;

            SupportPoint newPoint = ComputeDirectSupport(world, typeA, idxA, typeB, idxB, direction);

            // Check if we've passed the origin
            float projection = glm::dot(newPoint.csoPoint, direction);
            if (projection <= 0.0f) {
                return false;  // No intersection
            }

            // Add new point to simplex
            simplex.points[simplex.size++] = newPoint;

            // Process simplex
            bool containsOrigin = UpdateSimplex(simplex, direction);
            if (containsOrigin) {
                return RunEPAAlgorithm(world, simplex, typeA, idxA, typeB, idxB,
                                    outNormal, outPenetration, outPointA, outPointB);
            }

            // If we have a degenerate simplex, adjust the direction
            if (glm::length2(direction) < 1e-10f) {
                // Try a fall-back direction
                direction = glm::cross(simplex.points[1].csoPoint - simplex.points[0].csoPoint,
                                     glm::vec3(1.0f, 0.0f, 0.0f));
                if (glm::length2(direction) < 1e-10f) {
                    direction = glm::cross(simplex.points[1].csoPoint - simplex.points[0].csoPoint,
                                         glm::vec3(0.0f, 1.0f, 0.0f));
                }
            }
        }

        // If we reach max iterations, consider it a separation
        return RunEPAAlgorithm(world, simplex, typeA, idxA, typeB, idxB,
                        outNormal, outPenetration, outPointA, outPointB);
    }

    void NarrowPhase(PhysicsWorld& world,
                     const std::vector<std::pair<uint32_t, uint32_t>>& pairs) {
        world.contacts.clear();

        for (const auto& pair : pairs) {
            const auto& nodeA = world.aabbTree.nodes[pair.first];
            const auto& nodeB = world.aabbTree.nodes[pair.second];

            if (!nodeA.isLeaf || !nodeB.isLeaf) continue;

            uint32_t idxA = nodeA.colliderIndex;
            uint32_t idxB = nodeB.colliderIndex;
            ColliderType typeA = nodeA.type;
            ColliderType typeB = nodeB.type;

            uint32_t bodyA = GetBodyIndex(world, typeA, idxA);
            uint32_t bodyB = GetBodyIndex(world, typeB, idxB);

            if (bodyA == bodyB) continue;
            if (!world.bodyPool[bodyA].flags.active && !world.bodyPool[bodyB].flags.active) continue;

            glm::vec3 normal;
            float penetration;
            glm::vec3 pointA, pointB;

            if (GJKIntersect(world, typeA, idxA, typeB, idxB,
                             normal, penetration, pointA, pointB)) {

                ContactPoint contact;
                contact.normal = normal;
                contact.penetration = penetration;
                contact.pointA = pointA;
                contact.pointB = pointB;
                contact.bodyAIndex = bodyA;
                contact.bodyBIndex = bodyB;

                world.contacts.push_back(contact);
            }
        }
    }


}
