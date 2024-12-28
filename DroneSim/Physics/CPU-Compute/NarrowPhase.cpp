#include "NarrowPhase.h"
#include "ColliderSystem.h" // might need for collider data
#include <iostream>

namespace drosim::physics::cpu {
    bool GJKIntersect(PhysicsWorld& world, uint32_t colliderA, ColliderType typeA,
                      uint32_t colliderB, ColliderType typeB,
                      ContactPoint& outContact)
    {
        // Real GJK steps go here. Let's say we detect collision -> true
        Simplex simplex;
        simplex.size = 0;

        // If colliding, call EPA
        return EPACompute(world, simplex, colliderA, typeA, colliderB, typeB, outContact);
    }

    bool EPACompute(PhysicsWorld& world, const Simplex& simplex,
                    uint32_t colliderA, ColliderType typeA,
                    uint32_t colliderB, ColliderType typeB,
                    ContactPoint& outContact)
    {
        // Fill out contact data
        outContact.normal = glm::vec3(0.f, 1.f, 0.f);
        outContact.penetration = 0.1f;

        // For demonstration, pick the body’s center as contact points
        uint32_t bodyA, bodyB;
        if (typeA == ColliderType::Box) bodyA = world.boxPool[colliderA].bodyIndex;
        else bodyA = world.spherePool[colliderA].bodyIndex;
        if (typeB == ColliderType::Box) bodyB = world.boxPool[colliderB].bodyIndex;
        else bodyB = world.spherePool[colliderB].bodyIndex;

        outContact.pointA = world.bodyPool[bodyA].motion.position;
        outContact.pointB = world.bodyPool[bodyB].motion.position;
        outContact.bodyAIndex = bodyA;
        outContact.bodyBIndex = bodyB;

        return true;
    }

    void NarrowPhase(PhysicsWorld& world,
                     const std::vector<std::pair<uint32_t, uint32_t>>& pairs)
    {
        world.contacts.clear();

        for (auto& p : pairs) {
            auto& nA = world.aabbTree.nodes[p.first];
            auto& nB = world.aabbTree.nodes[p.second];

            uint32_t idxA = nA.colliderIndex;
            uint32_t idxB = nB.colliderIndex;
            ColliderType typeA = nA.type;
            ColliderType typeB = nB.type;

            // Find actual body indices
            uint32_t bodyA, bodyB;
            if (typeA == ColliderType::Box) bodyA = world.boxPool[idxA].bodyIndex;
            else bodyA = world.spherePool[idxA].bodyIndex;

            if (typeB == ColliderType::Box) bodyB = world.boxPool[idxB].bodyIndex;
            else bodyB = world.spherePool[idxB].bodyIndex;

            // same body => skip
            if (bodyA == bodyB) continue;

            // skip if both inactive
            auto& rbA = world.bodyPool[bodyA];
            auto& rbB = world.bodyPool[bodyB];
            if (!rbA.flags.active && !rbB.flags.active) continue;

            // GJK/EPA
            ContactPoint cp;
            if (GJKIntersect(world, idxA, typeA, idxB, typeB, cp)) {
                world.contacts.push_back(cp);
            }
        }
    }
}
