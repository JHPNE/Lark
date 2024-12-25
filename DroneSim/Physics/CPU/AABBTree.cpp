#include "AABBTree.h"
#include "Collider.h"
#include "RigidBody.h"

#include <queue>

namespace drosim::physics {
  AABBTree::~AABBTree() {
    if (m_root) {
      std::queue<AABBTreeNode*> nodeQueue;
      nodeQueue.push(m_root);

      while (!nodeQueue.empty()) {
        AABBTreeNode *n = nodeQueue.front();
        nodeQueue.pop();

        if (!n->IsLeaf()) {
          nodeQueue.push(n->children[0]);
          nodeQueue.push(n->children[1]);
        }

        delete n;
      }
    }
    m_root = nullptr;
  }

void AABBTree::Add(AABB* aabb) {
    if (!aabb) {
      std::cout << "Warning: Attempting to add null AABB\n";
      return;
    }

    // Get the Collider that owns this AABB
    Collider* collider = static_cast<Collider*>(aabb->userData);
    if (!collider) {
      std::cout << "Warning: AABB has no associated collider\n";
      return;
    }

    std::cout << "Adding AABB to tree for collider at position Y: "
              << collider->GetRigidBody()->GetPosition().y << "\n";

    if (!m_root) {
      std::cout << "Creating root node\n";
      m_root = new AABBTreeNode();
      m_root->SetLeaf(aabb, collider);
      m_root->UpdateAABB(m_margin);
      return;
    }

    // Create new leaf
    AABBTreeNode* leaf = new AABBTreeNode();
    leaf->SetLeaf(aabb, collider);
    leaf->UpdateAABB(m_margin);

    std::cout << "Inserting new leaf node\n";
    InsertNode(leaf, &m_root);
  }


  void AABBTree::InsertNode(AABBTreeNode *node, AABBTreeNode **parentLink) {
    AABBTreeNode *p = *parentLink;

    if (p->IsLeaf()) {
      AABBTreeNode *newBranch = new AABBTreeNode();
      newBranch->SetBranch(p, node);
      newBranch->UpdateAABB(m_margin);

      newBranch->parent = p->parent;
      *parentLink = newBranch;
    } else {
      const float currentVol0 = p->children[0]->fatAABB.Volume();
      const float currentVol1 = p->children[1]->fatAABB.Volume();

      const float combinedVol0 = p->children[0]->fatAABB.Union(node->fatAABB).Volume();
      const float combinedVol1 = p->children[1]->fatAABB.Union(node->fatAABB).Volume();

      const float diff0 = combinedVol0 - currentVol0;
      const float diff1 = combinedVol1 - currentVol1;

      if (diff0 < diff1) {
        InsertNode(node, &p->children[0]);
      } else {
        InsertNode(node, &p->children[1]);
      }
    }

    // recompute parent
    (*parentLink)->UpdateAABB(m_margin);
  }

  void AABBTree::Remove(AABB *aabb) {
    if (!aabb || !aabb->userData) return;

    AABBTreeNode *node = static_cast<AABBTreeNode*>(aabb->userData);
    if (!node) return;

    aabb->userData = nullptr;
    node->data = nullptr;

    RemoveNode(node);
  }

  void AABBTree::RemoveNode(AABBTreeNode *node) {
    AABBTreeNode *parent = node->parent;

    if (!parent) {
      delete node;
      m_root = nullptr;
      return;
    }

    AABBTreeNode *grandParent = parent->parent;
    AABBTreeNode *sibling = node->GetSibling();

    if (grandParent) {
      if (grandParent->children[0] == parent) {
        grandParent->children[0] = sibling;
      } else {
        grandParent->children[1] = sibling;
      }

      sibling->parent = grandParent;
      delete parent;
    } else {
      m_root = sibling;
      sibling->parent = nullptr;
      delete parent;
    }

    delete node;
  }

  void AABBTree::Update() {
    std::cout << "Updating AABB tree\n";

    if (!m_root) {
      std::cout << "Warning: Empty AABB tree\n";
      return;
    }

    // Count nodes before update
    int nodeCount = 0;
    std::queue<AABBTreeNode*> countQueue;
    countQueue.push(m_root);
    while (!countQueue.empty()) {
      AABBTreeNode* node = countQueue.front();
      countQueue.pop();
      nodeCount++;
      if (!node->IsLeaf()) {
        if (node->children[0]) countQueue.push(node->children[0]);
        if (node->children[1]) countQueue.push(node->children[1]);
      }
    }
    std::cout << "Tree contains " << nodeCount << " nodes before update\n";

    // Clear invalid node buffer
    m_invalidNodes.clear();

    // Gather all invalid nodes
    UpdateNodeHelper(m_root);

    std::cout << "Found " << m_invalidNodes.size() << " invalid nodes\n";

    // Re-insert them
    for (auto* node : m_invalidNodes) {
      // Output node info
      Collider* collider = static_cast<Collider*>(node->userData);
      if (collider && collider->GetRigidBody()) {
        std::cout << "Re-inserting node for body at Y: "
                 << collider->GetRigidBody()->GetPosition().y << "\n";
      }

      // Remove from current location
      RemoveNode(node);

      // Re-insert
      node->UpdateAABB(m_margin);
      InsertNode(node, &m_root);
    }

    m_invalidNodes.clear();
  }

  void AABBTree::UpdateNodeHelper(AABBTreeNode *node) {
    if (!node) return;

    if (node->IsLeaf())
    {
      AABB realBox(node->data->minPoint, node->data->maxPoint);
      if (!node->fatAABB.Contains(realBox))
      {
        m_invalidNodes.push_back(node);
      }
    }
    else
    {
      UpdateNodeHelper(node->children[0]);
      UpdateNodeHelper(node->children[1]);
      // Also re-update the branch node’s union
      node->UpdateAABB(m_margin);
    }
  }

  const ColliderPairList &AABBTree::ComputePairs() {
    m_pairs.clear();
    std::cout << "\nComputing collision pairs...\n";

    // Early-out if empty or single leaf
    if (!m_root || m_root->IsLeaf()) {
        std::cout << "No pairs possible (empty or single leaf)\n";
        return m_pairs;
    }

    // Clear flags
    ClearChildrenCrossFlagHelper(m_root);

    // Recurse
    ComputePairsHelper(m_root->children[0], m_root->children[1]);

    std::cout << "Found " << m_pairs.size() << " potential collision pairs\n";
    return m_pairs;
  }

  //-----------------------------------------------------------
  void AABBTree::ClearChildrenCrossFlagHelper(AABBTreeNode *node) {
      if (!node) return;
      node->childrenCrossed = false;
      if (!node->IsLeaf())
      {
          ClearChildrenCrossFlagHelper(node->children[0]);
          ClearChildrenCrossFlagHelper(node->children[1]);
      }
  }

  //-----------------------------------------------------------
  void AABBTree::CrossChildren(AABBTreeNode *node) {
      if (!node->childrenCrossed)
      {
          ComputePairsHelper(node->children[0], node->children[1]);
          node->childrenCrossed = true;
      }
  }

  //-----------------------------------------------------------
  // ComputePairsHelper
  //-----------------------------------------------------------
  void AABBTree::ComputePairsHelper(AABBTreeNode *n0, AABBTreeNode *n1) {
    if (!n0 || !n1) return;

    // If leaf vs leaf => direct check
    if (n0->IsLeaf() && n1->IsLeaf()) {
      if (n0->data->Intersects(*n1->data)) {
        // Find the collider that owns each AABB
        void* userDataA = n0->data->userData;
        void* userDataB = n1->data->userData;

        if (!userDataA || !userDataB) return;

        // userData should be the AABBTreeNode*
        AABBTreeNode* nodeA = static_cast<AABBTreeNode*>(userDataA);
        AABBTreeNode* nodeB = static_cast<AABBTreeNode*>(userDataB);

        if (!nodeA || !nodeB) return;

        m_pairs.push_back(AllocatePair(
            static_cast<Collider*>(nodeA->userData),
            static_cast<Collider*>(nodeB->userData)));
      }
      return;
    }

    // Same logic for branch vs leaf cases
    if (n0->IsLeaf() && !n1->IsLeaf()) {
      CrossChildren(n1);
      ComputePairsHelper(n0, n1->children[0]);
      ComputePairsHelper(n0, n1->children[1]);
      return;
    }

    if (!n0->IsLeaf() && n1->IsLeaf()) {
      CrossChildren(n0);
      ComputePairsHelper(n0->children[0], n1);
      ComputePairsHelper(n0->children[1], n1);
      return;
    }

    // Both are branches
    CrossChildren(n0);
    CrossChildren(n1);
    ComputePairsHelper(n0->children[0], n1->children[0]);
    ComputePairsHelper(n0->children[0], n1->children[1]);
    ComputePairsHelper(n0->children[1], n1->children[0]);
    ComputePairsHelper(n0->children[1], n1->children[1]);
  }

  //-----------------------------------------------------------
  // Pick(const glm::vec3&)
  //   - A simple point-in-AABB test for all leaves
  //-----------------------------------------------------------
  Collider* AABBTree::Pick(const glm::vec3 &point) const {
    if (!m_root) return nullptr;

    std::queue<AABBTreeNode*> queue;
    queue.push(m_root);

    while (!queue.empty()) {
      AABBTreeNode* node = queue.front();
      queue.pop();

      if (node->fatAABB.Contains(point)) {
        if (node->IsLeaf()) {
          if (node->data && node->data->Contains(point)) {
            // userData of the node points to the Collider
            return static_cast<Collider*>(node->userData);
          }
        } else {
          queue.push(node->children[0]);
          queue.push(node->children[1]);
        }
      }
    }

    return nullptr;
  }

  //-----------------------------------------------------------
  // Query(const AABB&, ColliderList &)
  //   - Return all colliders overlapping the input region
  //-----------------------------------------------------------
  void AABBTree::Query(const AABB &region, ColliderList &output) const {
    output.clear();
    if (!m_root) return;

    std::queue<AABBTreeNode*> queue;
    queue.push(m_root);

    while (!queue.empty()) {
      AABBTreeNode* node = queue.front();
      queue.pop();

      if (region.Intersects(node->fatAABB)) {
        if (node->IsLeaf()) {
          if (region.Intersects(*node->data)) {
            // userData of the node points to the Collider
            output.push_back(static_cast<Collider*>(node->userData));
          }
        } else {
          queue.push(node->children[0]);
          queue.push(node->children[1]);
        }
      }
    }
  }

  //-----------------------------------------------------------
  // RayCast(const Ray3&)
  //   - Basic BFS approach, skipping branches that can't
  //     yield a better intersection
  //-----------------------------------------------------------
  RayCastResult AABBTree::RayCast(const Ray3 &ray) const {
    RayCastResult result;
    result.hit = false;
    result.t = std::numeric_limits<float>::max();
    result.collider = nullptr;
    result.position = glm::vec3(0);
    result.normal = glm::vec3(0);

    if (!m_root) return result;

    std::queue<AABBTreeNode*> queue;
    queue.push(m_root);

    while (!queue.empty()) {
      AABBTreeNode* node = queue.front();
      queue.pop();

      float tminOut = 0.0f, tmaxOut = 0.0f;
      if (RayAABB(ray.pos, ray.dir, node->fatAABB, tminOut, tmaxOut)) {
        if (node->IsLeaf()) {
          // Get the collider from node's userData
          Collider* col = static_cast<Collider*>(node->userData);
          if (col) {
            float tCandidate = 0.0f;
            glm::vec3 nCandidate(0);

            if (col->RayCast(ray, tCandidate, nCandidate)) {
              if (!result.hit || tCandidate < result.t) {
                result.hit = true;
                result.collider = col;
                result.normal = nCandidate;
                result.t = tCandidate;
                result.position = ray.pos + (ray.dir * tCandidate);
              }
            }
          }
        } else {
          queue.push(node->children[0]);
          queue.push(node->children[1]);
        }
      }
    }

    return result;
  }
}