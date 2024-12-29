#include "NarrowPhase.h"
#include "ColliderSystem.h" // might need for collider data

#include <iomanip>
#include <iostream>

namespace drosim::physics::cpu {

    namespace {
        glm::mat3 GetRotationMatrix(const RigidBody& body) {
            return glm::mat3_cast(body.motion.orientation);
        }

        glm::mat3 GetInvRotationMatrix(const RigidBody& body) {
            return glm::transpose(glm::mat3_cast(body.motion.orientation));
        }

        static glm::vec3 GetBoxSupport(const PhysicsWorld& world, uint32_t colliderIndex, const glm::vec3& dirWorld) {
            const auto& box = world.boxPool[colliderIndex];
            const auto& body = world.bodyPool[box.bodyIndex];

            glm::mat3 irot = GetInvRotationMatrix(body);
            glm::mat3 rot = GetRotationMatrix(body);

            glm::vec3 localDir = irot * dirWorld;
            glm::vec3 sign(
                localDir.x >= 0.f ? 1.f : -1.f,
                localDir.y >= 0.f ? 1.f : -1.f,
                localDir.z >= 0.f ? 1.f : -1.f
            );

            glm::vec3 localSupport = box.localCenter + sign * box.halfExtents;

            glm::vec3 sup = body.motion.position + (body.motion.orientation * localSupport);
            return sup;
        }

        static glm::vec3 GetSphereSupport(const PhysicsWorld& world, uint32_t colliderIndex, const glm::vec3& dirWorld)
        {
            const auto& sphere = world.spherePool[colliderIndex];
            const auto& body = world.bodyPool[sphere.bodyIndex];

            glm::mat3 rot = GetRotationMatrix(body);

            float len2 = glm::dot(dirWorld, dirWorld);
            glm::vec3 dir = (len2 < 1e-8f)
                ? glm::vec3(1.f, 0.f, 0.f)
                : dirWorld / std::sqrt(len2);

            glm::vec3 localSupport = sphere.localCenter + dir * sphere.radius;
            glm::vec3 sup = body.motion.position + (rot * localSupport);
            return sup;
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

        static SupportPoint GetSupport(const PhysicsWorld& world,
                                       ColliderType typeA, uint32_t idxA,
                                       ColliderType typeB, uint32_t idxB,
                                       const glm::vec3& dir) {
            glm::vec3 pA = GetColliderSupport(world, typeA, idxA, dir);
            glm::vec3 pB = GetColliderSupport(world, typeB, idxB, -dir);

            SupportPoint sp;
            sp.pointA = pA;
            sp.pointB = pB;
            sp.minkowskiPoint = pA - pB;
            return sp;
        }

        // Simplex helper functions:
        static void SetSimplex(Simplex& simplex, const SupportPoint& a)
        {
            simplex.pts[0] = a;
            simplex.size = 1;
        }
        static void SetSimplex(Simplex& simplex, const SupportPoint& a, const SupportPoint& b)
        {
            simplex.pts[0] = a;
            simplex.pts[1] = b;
            simplex.size = 2;
        }
        static void SetSimplex(Simplex& simplex, const SupportPoint& a, const SupportPoint& b, const SupportPoint& c)
        {
            simplex.pts[0] = a;
            simplex.pts[1] = b;
            simplex.pts[2] = c;
            simplex.size = 3;
        }
        static void SetSimplex(Simplex& simplex,
                           const SupportPoint& a,
                           const SupportPoint& b,
                           const SupportPoint& c,
                           const SupportPoint& d)
        {
            simplex.pts[0] = a;
            simplex.pts[1] = b;
            simplex.pts[2] = c;
            simplex.pts[3] = d;
            simplex.size = 4;
        }

        static bool DoSimplex(Simplex& simplex, glm::vec3& direction) {
            switch (simplex.size) {
            case 1:
                // Single point
                direction = -simplex.pts[0].minkowskiPoint;
                break;
            case 2:
            {
                // Line segment
                auto& A = simplex.pts[1];
                auto& B = simplex.pts[0];
                glm::vec3 AB = B.minkowskiPoint - A.minkowskiPoint;
                glm::vec3 AO = -A.minkowskiPoint;
                direction = glm::cross(glm::cross(AB, AO), AB);

                // Handle degenerate case
                if (glm::length2(direction) < 1e-8f) {
                    direction = -A.minkowskiPoint;
                }
            }
                break;
            case 3:
            {
                // Triangle
                auto& A = simplex.pts[2];
                auto& B = simplex.pts[1];
                auto& C = simplex.pts[0];
                glm::vec3 AB = B.minkowskiPoint - A.minkowskiPoint;
                glm::vec3 AC = C.minkowskiPoint - A.minkowskiPoint;
                glm::vec3 AO = -A.minkowskiPoint;
                glm::vec3 ABC = glm::cross(AB, AC);

                // Determine if origin is in regions AB or AC
                glm::vec3 ABperp = glm::cross(ABC, AB);
                if (glm::dot(ABperp, AO) > 0.f) {
                    // Remove C
                    simplex.pts[0] = A;
                    simplex.pts[1] = B;
                    simplex.size = 2;
                    direction = glm::cross(glm::cross(AB, AO), AB);
                    return false;
                }

                glm::vec3 ACperp = glm::cross(AC, ABC);
                if (glm::dot(ACperp, AO) > 0.f) {
                    // Remove B
                    simplex.pts[0] = A;
                    simplex.pts[1] = C;
                    simplex.size = 2;
                    direction = glm::cross(glm::cross(AC, AO), AC);
                    return false;
                }

                // Origin is within the triangle
                if (glm::dot(ABC, AO) > 0.f) {
                    direction = ABC;
                }
                else {
                    // Ensure the normal points towards the origin
                    simplex.pts[0] = A;
                    simplex.pts[1] = C;
                    simplex.pts[2] = B;
                    simplex.size = 3;
                    direction = -ABC;
                }
            } break;
            case 4:
                // Tetrahedron
                return true;
            default:
                // Invalid simplex size
                direction = glm::vec3(1.f, 0.f, 0.f);
                return false;
            }
            return false;
        }

        static bool GJK(const PhysicsWorld& world,
             ColliderType typeA, uint32_t idxA,
             ColliderType typeB, uint32_t idxB,
             Simplex& outSimplex)
        {
            glm::vec3 posA, posB;
            {
                if (typeA == ColliderType::Box) posA = world.bodyPool[world.boxPool[idxA].bodyIndex].motion.position;
                else                            posA = world.bodyPool[world.spherePool[idxA].bodyIndex].motion.position;
                if (typeB == ColliderType::Box) posB = world.bodyPool[world.boxPool[idxB].bodyIndex].motion.position;
                else                            posB = world.bodyPool[world.spherePool[idxB].bodyIndex].motion.position;
            }
            glm::vec3 dir = posB - posA;
            if (glm::length2(dir) < 1e-6f) {
                dir = glm::vec3(1.f, 0.f, 0.f); // fallback
            }

            // 2) get first support
            SupportPoint initial = GetSupport(world, typeA, idxA, typeB, idxB, dir);
            SetSimplex(outSimplex, initial);

            // direction from new point to origin
            dir = -initial.minkowskiPoint;
            const int MAX_ITER = 64; // Increased from 32
            for (int i = 0; i < MAX_ITER; i++) {
                // 3) get new support in 'dir'
                SupportPoint sp = GetSupport(world, typeA, idxA, typeB, idxB, dir);
                // if dot(sp.minkowskiPoint, dir) <= 0 => no collision
                float d = glm::dot(sp.minkowskiPoint, dir);
                if (d <= 0.f) {
                    return false; // no intersection
                }

                // add sp to simplex
                if (outSimplex.size < 4) {
                    outSimplex.pts[outSimplex.size++] = sp;
                } else {
                    // Replace the furthest point
                    // Find the point with the maximum dot product with direction
                    float maxDot = glm::dot(outSimplex.pts[0].minkowskiPoint, dir);
                    int index = 0;
                    for (int j = 1; j < 4; j++) {
                        float dotProd = glm::dot(outSimplex.pts[j].minkowskiPoint, dir);
                        if (dotProd > maxDot) {
                            maxDot = dotProd;
                            index = j;
                        }
                    }
                    outSimplex.pts[index] = sp;
                }


                // 4) reduce simplex
                if (DoSimplex(outSimplex, dir)) {
                    // Check for coplanarity
                    if (outSimplex.size == 4) {
                        glm::vec3 A = outSimplex.pts[3].minkowskiPoint;
                        glm::vec3 B = outSimplex.pts[2].minkowskiPoint;
                        glm::vec3 C = outSimplex.pts[1].minkowskiPoint;
                        glm::vec3 D = outSimplex.pts[0].minkowskiPoint;

                        glm::vec3 AB = B - A;
                        glm::vec3 AC = C - A;
                        glm::vec3 AD = D - A;
                        glm::vec3 normal = glm::cross(AB, AC);
                        float volume = glm::dot(normal, AD);

                        if (std::abs(volume) < 1e-6f) {
                            // Coplanar simplex, treat as no collision
                            return false;
                        }

                        return true;
                    }
                    return false;
                }
            }
            // if we never enclosed origin, treat as no collision
            return false;
        }

        // Build a polytope from the final GJK simplex.
        // For a tetrahedron, we have up to 4 points.
        static void BuildInitialPolytope(const Simplex& simplex, EPAPoly& poly)
        {
            poly.vertices.clear();
            poly.faces.clear();

            if (simplex.size < 4) {
                return; // or throw error
            }

            for (int i = 0; i < simplex.size; i++) {
                poly.vertices.push_back(simplex.pts[i]);
            }

            static const int tetraIndices[4][3] = {
                {0,1,2},{0,1,3},{0,2,3},{1,2,3}
            };

            for (int f = 0; f < 4; f++) {
                EPAFace face;
                face.a = tetraIndices[f][0];
                face.b = tetraIndices[f][1];
                face.c = tetraIndices[f][2];

                glm::vec3 A = poly.vertices[face.a].minkowskiPoint;
                glm::vec3 B = poly.vertices[face.b].minkowskiPoint;
                glm::vec3 C = poly.vertices[face.c].minkowskiPoint;

                glm::vec3 AB = B - A;
                glm::vec3 AC = C - A;
                glm::vec3 N = glm::cross(AB, AC);
                float len2 = glm::length2(N);

                if (len2 > 1e-6f) { // Lowered threshold from 1e-12f to 1e-6f
                    N = glm::normalize(N);  // Normalize

                    // Calculate distance from origin to face plane
                    float dist = glm::dot(N, A);

                    // Ensure normal points outward from origin
                    if (dist < 0.0f) {
                        N = -N;
                        dist = -dist;

                        // Maintain CCW winding
                        std::swap(face.b, face.c);
                    }

                    face.normal = N;
                    face.distance = dist;
                    poly.faces.push_back(face);
                }
                else {
                    // Handle nearly degenerate face
                    // Optionally, add logging or alternative handling
                    std::cerr << "Warning: Degenerate face detected in initial polytope.\n";
                }
            }
        }

        static void AddPointToPolytope(EPAPoly& poly, int closestFaceIdx, const SupportPoint& newPt) {
            // We remove the faces that face the newPt (where dot(face.normal, newPt - faceVertex) > 0)
            // Then add new faces linking newPt to the "hole" boundary. This is a simplified version.

            std::vector<int> toRemove;
            std::vector<std::array<int,2>> edges; // store boundary edges

            // Step 1: Mark all faces that we can see from newPt
            for (int i = 0; i < (int)poly.faces.size(); i++) {
                auto& f = poly.faces[i];
                glm::vec3 A = poly.vertices[f.a].minkowskiPoint;
                glm::vec3 N = f.normal;

                if (glm::dot(N, newPt.minkowskiPoint - A) > 0.f) {
                    toRemove.push_back(i);
                }
            }


            auto AddEdge = [&](int a, int b) {
                if (a > b) std::swap(a, b);
                edges.push_back({a, b});
            };

            std::vector<EPAFace> kept;
            kept.reserve(poly.faces.size());
            for (int i = 0; i < (int)poly.faces.size(); i++) {
                if (std::find(toRemove.begin(), toRemove.end(), i) == toRemove.end()) {
                    kept.push_back(poly.faces[i]);
                } else {
                    auto& ff = poly.faces[i];
                    AddEdge(ff.a, ff.b);
                    AddEdge(ff.b, ff.c);
                    AddEdge(ff.c, ff.a);
                }
            }

            poly.faces = kept;

            auto edgeCount = [&](std::array<int,2> e) {
                int count = 0;
                for (int r : toRemove) {
                    auto& ff = poly.faces[r];
                }
                return count;
            };

            std::vector<bool> removed(edges.size(), false);
            for (int i = 0; i < (int)edges.size(); i++) {
                for (int j = i + 1; j < (int)edges.size(); j++) {
                    if (edges[i] == edges[j]) {
                        removed[i] = true;
                        removed[j] = true;
                    }
                }
            }

            std::vector<std::array<int,2>> border;
            for (int i = 0; i < (int)edges.size(); i++) {
                if (!removed[i]) {
                    border.push_back(edges[i]);
                }
            }

            int newIndex = (int)poly.vertices.size();
            poly.vertices.push_back(newPt);

            // create new faces
            for (auto& e : border) {
                EPAFace face;
                face.a = newIndex;
                face.b = e[0];
                face.c = e[1];

                glm::vec3 A = poly.vertices[face.a].minkowskiPoint;
                glm::vec3 B = poly.vertices[face.b].minkowskiPoint;
                glm::vec3 C = poly.vertices[face.c].minkowskiPoint;


                glm::vec3 AB = B - A;
                glm::vec3 AC = C - A;
                glm::vec3 N = glm::cross(AB, AC);
                float len2 = glm::length2(N);

                if (len2 > 1e-12f) {
                    N = glm::normalize(N);
                }
                float dist = glm::dot(N, A);
                if (dist > 0.f) {
                    N = -N;
                    dist = -dist;

                    std::swap(face.b, face.c);
                }
                face.normal = N;
                face.distance = dist;
                poly.faces.push_back(face);
            }
        }

        // EPA main function
        static bool EPA(const PhysicsWorld& world,
                       ColliderType typeA, uint32_t idxA,
                       ColliderType typeB, uint32_t idxB,
                       Simplex& simplex,
                       glm::vec3& outNormal,
                       float& outPenetration)
        {
            EPAPoly poly;
            BuildInitialPolytope(simplex, poly);

            // If we don't have enough faces, try to recover using simplex
            if (poly.faces.size() < 4) {
                // Fallback: Unable to form a valid tetrahedron
                std::cout << "EPA Error: Initial polytope has less than 4 faces.\n";
                return false;
            }

            const int MAX_ITER = 64;
            const float TOL = 1e-4f;
            float prevDist = FLT_MAX;

            for (int iter = 0; iter < MAX_ITER; iter++) {
                int closestFaceIdx = -1;
                float minDist = FLT_MAX;

                for (size_t i = 0; i < (int)poly.faces.size(); i++) {
                    auto& face = poly.faces[i];
                    if (face.distance < minDist) {
                        minDist = face.distance;
                        closestFaceIdx = i;
                    }
                }
                if (closestFaceIdx < 0) {
                    return false;
                }

                auto& closestFace = poly.faces[closestFaceIdx];
                outNormal = closestFace.normal;
                outPenetration = closestFace.distance; // distance is negative

                // see if we can expand
                SupportPoint sp = GetSupport(world, typeA, idxA, typeB, idxB, outNormal);
                float d = glm::dot(sp.minkowskiPoint, outNormal);

                if (std::abs(d - minDist) < TOL) {
                    return true;
                }

                // Expand polytope
                AddPointToPolytope(poly, closestFaceIdx, sp);
                prevDist = minDist;
            }

            // If we reach here, use best result found
            if (!poly.faces.empty()) {
                float minDist = FLT_MAX;
                for (const auto& face : poly.faces) {
                    if (face.distance < minDist) {
                        minDist = face.distance;
                        outNormal = face.normal;
                        outPenetration = face.distance;
                    }
                }
                return true;
            }

            return false;
        }
    } // anonymous namespace


bool GJKIntersect(const PhysicsWorld& world,
               ColliderType typeA, uint32_t idxA,
               ColliderType typeB, uint32_t idxB,
               glm::vec3& outNormal,
               float& outPenetration,
               glm::vec3& outPointA,
               glm::vec3& outPointB)
    {
        Simplex simplex;
        if (!GJK(world, typeA, idxA, typeB, idxB, simplex)) {
            return false;
        }

        if (!EPA(world, typeA, idxA, typeB, idxB, simplex, outNormal, outPenetration)) {
            return false;
        }

        // Get body positions
        uint32_t bodyA = (typeA == ColliderType::Box)
            ? world.boxPool[idxA].bodyIndex
            : world.spherePool[idxA].bodyIndex;
        uint32_t bodyB = (typeB == ColliderType::Box)
            ? world.boxPool[idxB].bodyIndex
            : world.spherePool[idxB].bodyIndex;

        const auto& posA = world.bodyPool[bodyA].motion.position;
        const auto& posB = world.bodyPool[bodyB].motion.position;

        // Ensure normal points from A to B
        glm::vec3 AB = posB - posA;
        if (glm::dot(outNormal, AB) < 0.0f) {
            outNormal = -outNormal;
        }

        float halfPen = 0.5f * outPenetration;
        outPointA = posA + outNormal * halfPen;
        outPointB = posB - outNormal * halfPen;

        return true;
    }

    void NarrowPhase(PhysicsWorld& world,
                     const std::vector<std::pair<uint32_t, uint32_t>>& pairs)
    {
        world.contacts.clear();

        for (auto& p : pairs) {
            auto& nA = world.aabbTree.nodes[p.first];
            auto& nB = world.aabbTree.nodes[p.second];
            if (!nA.isLeaf || !nB.isLeaf) continue; // skipping non-leaf nodes

            uint32_t idxA = nA.colliderIndex;
            uint32_t idxB = nB.colliderIndex;
            ColliderType typeA = nA.type;
            ColliderType typeB = nB.type;

            // Find actual body indices
            uint32_t bodyA = (typeA == ColliderType::Box)
                                ? world.boxPool[idxA].bodyIndex
                                : world.spherePool[idxA].bodyIndex;
            uint32_t bodyB = (typeB == ColliderType::Box)
                                ? world.boxPool[idxB].bodyIndex
                                : world.spherePool[idxB].bodyIndex;

            // skip if same body or both inactive
            if (bodyA == bodyB) continue;
            if (!world.bodyPool[bodyA].flags.active && !world.bodyPool[bodyB].flags.active)
                continue;

            glm::vec3 normal;
            float penetration;
            glm::vec3 pointA, pointB;
            bool hit = GJKIntersect(world, typeA, idxA, typeB, idxB, normal, penetration, pointA, pointB);

            // TODO precision settings
            if (hit && penetration > 1e-5f) {
                ContactPoint cp{};
                cp.normal = glm::normalize(normal);
                cp.penetration = penetration;
                cp.pointA = pointA;
                cp.pointB = pointB;
                cp.bodyAIndex = bodyA;
                cp.bodyBIndex = bodyB;

                world.contacts.push_back(cp);
            }
        }
    }
}
