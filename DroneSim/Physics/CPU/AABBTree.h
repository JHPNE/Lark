#pragma once
#include "Broadphase.h"
#include "Colliders/BoxCollider.h"
#include "PhysicsStructures.h" // your bounding box class/struct
#include "RigidBody.h"

#include <glm/glm.hpp>         // for glm::vec3, etc., if you need it
#include <list>
#include <vector>

namespace drosim::physics {
  struct AABBTreeNode;

  class AABBTree : public Broadphase {
    public:
      AABBTree(float margin = 0.2f) : m_root(nullptr), m_margin(margin) {}

      virtual ~AABBTree();

      void Add(AABB *aabb) override;
      void Remove(AABB *aabb) override;

      void Update() override;
      const ColliderPairList &ComputePairs() override;

      Collider *Pick(const glm::vec3 &point) const;

      void Query(const AABB &region, ColliderList &output) const override;

      RayCastResult RayCast(const Ray3 &ray) const override;

    private:
      struct AABBTreeNode {
        AABBTreeNode *parent;
        AABBTreeNode *children[2];
        bool childrenCrossed; // used to avoid duplicate pairs
        AABB fatAABB; // fat aabb if leaf or union of children
        AABB* data;
        void* userData;

        AABBTreeNode()
          : parent(nullptr),
            children{nullptr, nullptr},
            childrenCrossed(false),
            data(nullptr),
            userData(nullptr)
        {}

        bool IsLeaf() const {
          return (children[0] == nullptr);
        }

        void SetBranch(AABBTreeNode *n0, AABBTreeNode *n1) {
          children[0] = n0;
          children[1] = n1;
          n0->parent = this;
          n1->parent = this;
        }

        void SetLeaf(AABB* aabb, Collider* collider) {
          std::cout << "SetLeaf called\n";
          std::cout << "AABB: " << (aabb ? "Valid" : "Null") << "\n";
          std::cout << "Collider: " << (collider ? "Valid" : "Null") << "\n";

          data = aabb;
          userData = collider;
          if (data) {
            // Store the AABBTreeNode pointer in the AABB
            data->userData = this;
            // Copy bounds to fatAABB with margin
            const float margin = 0.1f;
            const glm::vec3 marginVec(margin);
            fatAABB.minPoint = data->minPoint - marginVec;
            fatAABB.maxPoint = data->maxPoint + marginVec;

            std::cout << "Leaf node bounds set to Y=["
                      << fatAABB.minPoint.y << ", "
                      << fatAABB.maxPoint.y << "]\n";
          }
        }

        void UpdateAABB(float margin) {
          if (IsLeaf()) {
            if (data) {
              Collider* collider = static_cast<Collider*>(userData);
              if (collider && collider->GetRigidBody()) {
                // Get current body position
                glm::vec3 pos = collider->GetRigidBody()->GetPosition();
                const BoxCollider* boxCollider = dynamic_cast<const BoxCollider*>(collider);
                if (boxCollider) {
                  glm::vec3 halfExtents = boxCollider->m_shape.m_halfExtents;
                  // Update actual AABB
                  data->minPoint = pos - halfExtents;
                  data->maxPoint = pos + halfExtents;
                  // Update fat AABB
                  fatAABB.minPoint = data->minPoint - glm::vec3(margin);
                  fatAABB.maxPoint = data->maxPoint + glm::vec3(margin);

                  std::cout << "Updated leaf node at Y=" << pos.y
                          << " AABB=[" << data->minPoint.y << ", " << data->maxPoint.y
                          << "] Fat=[" << fatAABB.minPoint.y << ", " << fatAABB.maxPoint.y << "]\n";
                }
              }
            }
          } else {
            if (children[0] && children[1]) {
              // Update children first
              children[0]->UpdateAABB(margin);
              children[1]->UpdateAABB(margin);
              // Then update this node's bounds
              fatAABB = children[0]->fatAABB.Union(children[1]->fatAABB);
            }
          }
        }

        AABBTreeNode *GetSibling() const {
          if (!parent) return nullptr;
          return (this == parent->children[0]) ? parent->children[1] : parent->children[0];
        }
      };

    void InsertNode(AABBTreeNode *node, AABBTreeNode **parentLink);
    void RemoveNode(AABBTreeNode *node);

    // Helpers
    void UpdateNodeHelper(AABBTreeNode *node);
    void ComputePairsHelper(AABBTreeNode *n0, AABBTreeNode *n1);
    void ClearChildrenCrossFlagHelper(AABBTreeNode *node);
    void CrossChildren(AABBTreeNode *node);

    // Utilities to allocate a new pair
    // (you might do this differently in your own code)
    inline ColliderPair AllocatePair(Collider *c0, Collider *c1) const
    {
      return std::make_pair(c0, c1);
    }

    AABBTreeNode *m_root;
    float m_margin;
    mutable ColliderPairList m_pairs;

    std::vector<AABBTreeNode*> m_invalidNodes;
  };
}