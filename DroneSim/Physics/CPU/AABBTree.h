#pragma once
#include "PhysicsStructures.h"       // your bounding box class/struct
#include <list>
#include <vector>
#include <glm/glm.hpp> // for glm::vec3, etc., if you need it
#include "Broadphase.h"

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

        void SetLeaf(AABB *aabb, Collider* collider) {
          data = aabb;
          userData = collider;
          if (data) {
            data->userData = this;
          }
        }

        void UpdateAABB(float margin) {
          if (IsLeaf()) {
            if (data) {
              const glm::vec3 marginVec(margin);
              fatAABB.minPoint = data->minPoint - marginVec;
              fatAABB.maxPoint = data->maxPoint + marginVec;
            }
          } else {
            fatAABB = children[0]->fatAABB.Union(children[1]->fatAABB);
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