#include "ColliderSystem.h"
#include <algorithm>
#include <iostream>
#include <omp.h>
#include <ostream>

namespace drosim::physics::cpu {

    inline glm::mat3 GetRotationMatrix(const RigidBody& rb) {
        return glm::mat3_cast(rb.motion.orientation);
    }

    uint32_t CreateBoxCollider(PhysicsWorld& world, uint32_t bodyIndex,
                                 const glm::vec3& halfExtents,
                                 const glm::vec3& localCenter) {
        uint32_t idx = static_cast<uint32_t>(world.boxPool.Allocate());
        auto& box = world.boxPool[idx];
        box.bodyIndex = bodyIndex;
        box.localCenter = localCenter;
        box.halfExtents = halfExtents;

        // Insert in AABB tree as a leaf
        // For now, we pass dummy AABB; we’ll update in next frame
        glm::vec3 dummyMin(0.f), dummyMax(0.f);
        InsertLeafNode(world.aabbTree, dummyMin, dummyMax, ColliderType::Box, idx);

        return idx;
    }

    uint32_t CreateSphereCollider(PhysicsWorld& world, uint32_t bodyIndex,
                                    float radius,
                                    const glm::vec3& localCenter) {
        uint32_t idx = static_cast<uint32_t>(world.spherePool.Allocate());
        auto& sphere = world.spherePool[idx];
        sphere.bodyIndex = bodyIndex;
        sphere.localCenter = localCenter;
        sphere.radius = radius;

        glm::vec3 dummyMin(0.f), dummyMax(0.f);
        InsertLeafNode(world.aabbTree, dummyMin, dummyMax, ColliderType::Sphere, idx);

        return idx;
    }

    void UpdateDynamicTree(PhysicsWorld& world) {
        auto& tree = world.aabbTree;
        for (size_t i = 0; i < tree.nodes.size(); ++i) {
            if (tree.nodes[i].isLeaf) {
                auto& node = tree.nodes[i];
                uint32_t cIndex = node.colliderIndex;
                ColliderType cType = node.type;

                glm::vec3 minPt(99999.f), maxPt(-99999.f);

                if (cType == ColliderType::Box) {
                    auto& box = world.boxPool[cIndex];
                    auto& body = world.bodyPool[box.bodyIndex];
                    if (body.flags.active || body.flags.isStatic) {
                        glm::mat3 rot = GetRotationMatrix(body);
                        glm::vec3 pos = body.motion.position + rot * box.localCenter;
                        glm::vec3 x = rot[0] * box.halfExtents.x;
                        glm::vec3 y = rot[1] * box.halfExtents.y;
                        glm::vec3 z = rot[2] * box.halfExtents.z;
                        glm::vec3 r(
                            std::abs(x.x) + std::abs(y.x) + std::abs(z.x),
                            std::abs(x.y) + std::abs(y.y) + std::abs(z.y),
                            std::abs(x.z) + std::abs(y.z) + std::abs(z.z)
                        );
                        minPt = pos - r;
                        maxPt = pos + r;
                    }
                }
                else if (cType == ColliderType::Sphere) {
                    auto& sph = world.spherePool[cIndex];
                    auto& body = world.bodyPool[sph.bodyIndex];
                    if (body.flags.active || body.flags.isStatic) {
                        glm::mat3 rot = GetRotationMatrix(body); // not used for radius, but for local center
                        glm::vec3 pos = body.motion.position + rot * sph.localCenter;
                        glm::vec3 r(sph.radius);
                        minPt = pos - r;
                        maxPt = pos + r;
                    }
                }

                // expand by margin
                glm::vec3 expand(tree.margin);
                minPt -= expand;
                maxPt += expand;

                UpdateLeafNode(tree, (uint32_t)i, minPt, maxPt);
            }
        }
        RebalanceAABB(tree);
    }

    void BroadPhaseCollisions(PhysicsWorld& world, std::vector<std::pair<uint32_t, uint32_t>>& outPairs) {
        auto& tree = world.aabbTree;
        // Collect all leaves
        std::vector<uint32_t> leaves;
        leaves.reserve(tree.nodes.size());
        for (size_t i = 0; i < tree.nodes.size(); ++i) {
            if (tree.nodes[i].isLeaf) {
                leaves.push_back((uint32_t)i);
            }
        }

        // For each leaf, do a tree query
        for (uint32_t leaf : leaves) {
            auto& nodeA = tree.nodes[leaf];
            // skip if invalid
            if (nodeA.minPoint.x > nodeA.maxPoint.x) continue;

            std::vector<uint32_t> stack;
            stack.push_back(tree.root);

            while (!stack.empty()) {
                uint32_t index = stack.back();
                stack.pop_back();

                if (index == UINT32_MAX) continue;
                auto& nodeB = tree.nodes[index];

                if (!( nodeB.maxPoint.x < nodeA.minPoint.x
                 || nodeB.minPoint.x > nodeA.maxPoint.x
                 || nodeB.maxPoint.y < nodeA.minPoint.y
                 || nodeB.minPoint.y > nodeA.maxPoint.y
                 || nodeB.maxPoint.z < nodeA.minPoint.z
                 || nodeB.minPoint.z > nodeA.maxPoint.z ))
                {
                    // So we DO overlap
                    if (nodeB.isLeaf && index != leaf) {
                        // record pair
                        outPairs.push_back({ leaf, index });
                    } else {
                        // descend
                        stack.push_back(nodeB.children[0]);
                        stack.push_back(nodeB.children[1]);
                    }
                }
            }
        }
    }

    uint32_t InsertLeafNode(DynamicAABBTree& tree, const glm::vec3& minPt, const glm::vec3& maxPt,
                    ColliderType type, uint32_t colliderIndex)
    {
        uint32_t nodeIdx;
        if (!tree.freeList.empty()) {
            nodeIdx = tree.freeList.back();
            tree.freeList.pop_back();
            if (nodeIdx >= tree.nodes.size()) {
                nodeIdx = (uint32_t)tree.nodes.size();
                tree.nodes.emplace_back();
            }
        } else {
            nodeIdx = (uint32_t)tree.nodes.size();
            tree.nodes.emplace_back();
        }

        auto& node = tree.nodes[nodeIdx];
        node.minPoint = minPt;
        node.maxPoint = maxPt;
        node.isLeaf = true;
        node.parent = UINT32_MAX;
        node.children[0] = UINT32_MAX;
        node.children[1] = UINT32_MAX;
        node.type = type;
        node.colliderIndex = colliderIndex;

        // Insert into the tree
        if (tree.root == UINT32_MAX) {
            tree.root = nodeIdx;
        } else {
            // naive approach
            uint32_t oldRoot = tree.root;
            uint32_t newParentIdx;
            if (!tree.freeList.empty()) {
                newParentIdx = tree.freeList.back();
                tree.freeList.pop_back();
                if (newParentIdx >= tree.nodes.size()) {
                    newParentIdx = (uint32_t)tree.nodes.size();
                    tree.nodes.emplace_back();
                }
            } else {
                newParentIdx = (uint32_t)tree.nodes.size();
                tree.nodes.emplace_back();
            }

            auto& newParent = tree.nodes[newParentIdx];
            newParent.isLeaf = false;
            newParent.parent = UINT32_MAX;
            newParent.children[0] = oldRoot;
            newParent.children[1] = nodeIdx;
            tree.root = newParentIdx;

            tree.nodes[oldRoot].parent = newParentIdx;
            node.parent = newParentIdx;
        }

        return nodeIdx;
    }

    void UpdateLeafNode(DynamicAABBTree& tree, uint32_t nodeIndex,
                const glm::vec3& minPt, const glm::vec3& maxPt)
    {
        auto& node = tree.nodes[nodeIndex];
        node.minPoint = minPt;
        node.maxPoint = maxPt;

        // bubble up changes
        uint32_t parent = node.parent;
        while (parent != UINT32_MAX) {
            auto& p = tree.nodes[parent];
            glm::vec3 cmin(99999.f), cmax(-99999.f);
            for (int i = 0; i < 2; i++) {
                uint32_t c = p.children[i];
                if (c != UINT32_MAX) {
                    auto& child = tree.nodes[c];
                    cmin = glm::min(cmin, child.minPoint);
                    cmax = glm::max(cmax, child.maxPoint);
                }
            }
            p.minPoint = cmin;
            p.maxPoint = cmax;
            parent = p.parent;
        }
    }

    void RebalanceAABB(DynamicAABBTree& tree) {
        // Not a full dynamic rebalance;
        // You can implement a surface-area heuristic, rotations, etc.
    }
}