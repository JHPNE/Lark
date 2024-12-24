#include "AABBTree.h"
#include <assert.h> // for assert()
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

  void AABBTree::Add(AABB *aabb) {
    if (!m_root) {
      m_root = new AABBTreeNode();
      m_root->SetLeaf(aabb);
      m_root->UpdateAABB(m_margin);
      return;
    }

    AABBTreeNode *leaf = new AABBTreeNode();
    leaf->SetLeaf(aabb);
    leaf->UpdateAABB(m_margin);

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
    if (!m_root)
      return;

    // Clear invalid node buffer
    m_invalidNodes.clear();

    // gather all invalid nodes
    UpdateNodeHelper(m_root);

    // Re-insert them
    for (auto *node : m_invalidNodes)
    {
      // remove from current location in tree
      RemoveNode(node);

      // re-insert
      node->UpdateAABB(m_margin);
      InsertNode(node, &m_root);
    }

    // done
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

    // Early-out if empty or single leaf
    if (!m_root || m_root->IsLeaf())
        return m_pairs;

    // Clear flags
    ClearChildrenCrossFlagHelper(m_root);

    // Recurse
    ComputePairsHelper(m_root->children[0], m_root->children[1]);

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
      if (n0->IsLeaf() && n1->IsLeaf())
      {
          // We check the *actual* bounding boxes:
          // If they overlap, add to pair list
          if (n0->data->Collides(*n1->data))
          {
              Collider *c0 = n0->data->colliderPtr;
              Collider *c1 = n1->data->colliderPtr;

              if (c0 && c1) {
                m_pairs.push_back(AllocatePair(c0, c1));
              }
          }
          return;
      }

      // if n1 is branch but n0 is leaf => cross children of n1
      if (n0->IsLeaf() && !n1->IsLeaf())
      {
          CrossChildren(n1);
          ComputePairsHelper(n0, n1->children[0]);
          ComputePairsHelper(n0, n1->children[1]);
          return;
      }

      // if n0 is branch but n1 is leaf => cross children of n0
      if (!n0->IsLeaf() && n1->IsLeaf())
      {
          CrossChildren(n0);
          ComputePairsHelper(n0->children[0], n1);
          ComputePairsHelper(n0->children[1], n1);
          return;
      }

      // else => both are branches => cross children of both
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
  Collider *AABBTree::Pick(const glm::vec3 &point) const {
      if (!m_root)
          return nullptr;

      // We’ll just do a BFS over the tree
      // and see if we find a leaf that contains the point
      std::queue<AABBTreeNode*> queue;
      queue.push(m_root);

      while (!queue.empty())
      {
          AABBTreeNode *node = queue.front();
          queue.pop();

          // If the ray or point intersects the node's bounding box
          if (node->fatAABB.Contains(point))
          {
              // If leaf => we can do a more exact check or just return
              if (node->IsLeaf())
              {
                  // Check the leaf’s real AABB
                  if (node->data && node->data->Contains(point))
                  {
                      // Return the first found
                      return node->data->Collider();
                  }
              }
              else
              {
                  // Not a leaf => push children
                  queue.push(node->children[0]);
                  queue.push(node->children[1]);
              }
          }
      }

      // no match
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

      while (!queue.empty())
      {
          AABBTreeNode *node = queue.front();
          queue.pop();

          // If the region intersects the node's bounding box
          if (region.Collides(node->fatAABB))
          {
              if (node->IsLeaf())
              {
                  // Check the leaf’s *real* bounding box
                  if (region.Collides(*node->data))
                  {
                      output.push_back(node->data->Collider());
                  }
              }
              else
              {
                  // push children to check further
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
      result.hit      = false;
      result.t        = 0.f;
      result.collider = nullptr;
      result.position = glm::vec3(0);
      result.normal   = glm::vec3(0);

      if (!m_root)
          return result;

      // We do BFS
      std::queue<AABBTreeNode*> queue;
      queue.push(m_root);

      // In a more optimized approach, you’d keep track
      // of the nearest t so far, then skip entire branches
      // whose intersection parameter > that t
      // but let's keep it simple:

      while (!queue.empty())
      {
          AABBTreeNode *node = queue.front();
          queue.pop();

          // Does the ray intersect the node's bounding box at all?
          float tminOut = 0.f, tmaxOut = 0.f;
          if (RayAABB(ray.pos, ray.dir, node->fatAABB, tminOut, tmaxOut)) {
              // if leaf => do precise collider-level ray cast
              if (node->IsLeaf() && node->data) {
                  // We'll assume your collider can do ray-cast
                  // returning intersection info in a local struct or so
                  Collider *col = node->data->colliderPtr;
                  if (col) {
                    float tCandidate = 0.f;
                    glm::vec3 nCandidate(0);

                    if (col->RayCast(ray, tCandidate, nCandidate)) {
                        // If first hit or if tCandidate is better (closer)
                        if (!result.hit || tCandidate < result.t) {
                            result.hit      = true;
                            result.collider = node->data->Collider();
                            result.normal   = nCandidate;
                            result.t        = tCandidate;
                            result.position = ray.pos + (ray.dir * tCandidate);
                        }
                    }
                  }
              } else {
                // branch => check children
                queue.push(node->children[0]);
                queue.push(node->children[1]);
              }
          }
      }

      return result;
  }
}