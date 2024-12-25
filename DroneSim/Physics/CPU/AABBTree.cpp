#include "AABBTree.h"
#include "Collider.h"
#include "Colliders/BoxCollider.h"
#include "RigidBody.h"

#include <queue>
#include <unordered_map>

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
    std::cout << "AABBTree::Add called\n";

    if (!aabb) {
      std::cout << "Warning: Attempting to add null AABB\n";
      return;
    }

    // Debug the AABB state
    std::cout << "AABB bounds: Y=[" << aabb->minPoint.y
              << ", " << aabb->maxPoint.y << "]\n";
    std::cout << "AABB userData: " << (aabb->userData ? "Set" : "Null") << "\n";

    if (!m_root) {
      std::cout << "Creating root node\n";
      m_root = new AABBTreeNode();
      m_root->SetLeaf(aabb, static_cast<Collider*>(aabb->userData));
      m_root->UpdateAABB(m_margin);
      std::cout << "Root node created with bounds Y=["
                << m_root->fatAABB.minPoint.y << ", "
                << m_root->fatAABB.maxPoint.y << "]\n";
      return;
    }

    // Create new leaf
    AABBTreeNode* leaf = new AABBTreeNode();
    leaf->SetLeaf(aabb, static_cast<Collider*>(aabb->userData));
    leaf->UpdateAABB(m_margin);

    std::cout << "Inserting new leaf node with bounds Y=["
              << leaf->fatAABB.minPoint.y << ", "
              << leaf->fatAABB.maxPoint.y << "]\n";

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
    if (!m_root) {
        std::cout << "Warning: Empty AABB tree\n";
        return;
    }

    // Print tree state
    std::cout << "Updating AABB tree structure\n";
    std::cout << "Root is " << (m_root->IsLeaf() ? "leaf" : "branch") << "\n";

    m_invalidNodes.clear();
    std::unordered_map<AABBTreeNode *, bool> processedNodes;

    // Start with leaves
    std::queue<AABBTreeNode*> nodeQueue;
    nodeQueue.push(m_root);

    while (!nodeQueue.empty()) {
        AABBTreeNode* node = nodeQueue.front();
        nodeQueue.pop();

        if (node->IsLeaf()) {
            // Update leaf node's fat AABB based on actual AABB
            if (node->data) {
                glm::vec3 oldMin = node->fatAABB.minPoint;
                glm::vec3 oldMax = node->fatAABB.maxPoint;

                // Expand by margin
                node->fatAABB.minPoint = node->data->minPoint - glm::vec3(m_margin);
                node->fatAABB.maxPoint = node->data->maxPoint + glm::vec3(m_margin);

                std::cout << "Leaf node AABB moved from Y=[" << oldMin.y << ", " << oldMax.y
                         << "] to Y=[" << node->fatAABB.minPoint.y
                         << ", " << node->fatAABB.maxPoint.y << "]\n";
            }
        } else {
            if (node->children[0]) nodeQueue.push(node->children[0]);
            if (node->children[1]) nodeQueue.push(node->children[1]);

            // Update branch node's AABB to contain children
            if (node->children[0] && node->children[1] && !processedNodes[node]) {
                node->fatAABB = node->children[0]->fatAABB.Union(
                    node->children[1]->fatAABB);
                processedNodes[node] = true;

                std::cout << "Branch node AABB updated to Y=["
                         << node->fatAABB.minPoint.y << ", "
                         << node->fatAABB.maxPoint.y << "]\n";
            }
        }
    }
  }

  void AABBTree::UpdateNodeHelper(AABBTreeNode *node) {
    if (!node) return;

    if (node->IsLeaf()) {
      // Get the actual AABB from the body
      AABB realBox;
      Collider* collider = static_cast<Collider*>(node->userData);
      if (collider && collider->GetRigidBody()) {
        glm::vec3 pos = collider->GetRigidBody()->GetPosition();
        glm::vec3 halfExtents;
        const BoxCollider* boxCollider = dynamic_cast<const BoxCollider*>(collider);
        if (boxCollider) {
          halfExtents = boxCollider->m_shape.m_halfExtents;
        }

        // Update the actual AABB
        node->data->minPoint = pos - halfExtents;
        node->data->maxPoint = pos + halfExtents;

        std::cout << "Updated leaf node AABB at Y=" << pos.y
                 << " Bounds: [" << node->data->minPoint.y
                 << ", " << node->data->maxPoint.y << "]\n";

        // Update the fat AABB
        realBox = *node->data;
        if (!node->fatAABB.Contains(realBox)) {
          m_invalidNodes.push_back(node);
        }
      }
    } else {
      UpdateNodeHelper(node->children[0]);
      UpdateNodeHelper(node->children[1]);
      if (node->children[0] && node->children[1]) {
        node->fatAABB = node->children[0]->fatAABB.Union(node->children[1]->fatAABB);
      }
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