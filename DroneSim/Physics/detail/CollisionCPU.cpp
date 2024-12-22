#include "CollisionCPU.h"

bool BroadphaseCPU::containedIn(const AABB& container, const AABB& inner) {
  return (container.min.x <= inner.min.x && container.min.y <= inner.min.y && container.min.z <= inner.min.z &&
          container.max.x >= inner.max.x && container.max.y >= inner.max.y && container.max.z >= inner.max.z);
}

AABB BroadphaseCPU::getStoredAABB(int bodyIndex) {
  if (bodyIndex < (int)tree.leafNodes.size() && tree.leafNodes[bodyIndex]) {
    return tree.leafNodes[bodyIndex]->bounds;
  }
  // If not found, return a default AABB (or handle error)
  return {{0,0,0},{0,0,0}};
}

Pair BroadphaseCPU::makePair(int a, int b) {
  return (a < b) ? Pair{a,b} : Pair{b,a};
}

bool BroadphaseCPU::narrowphase(const CollisionBody& A, const CollisionBody& B) {
  // circle-sphere test in 3D
  float dx = A.position.x - B.position.x;
  float dy = A.position.y - B.position.y;
  float dz = A.position.z - B.position.z;
  float rr = (A.radius + B.radius) * (A.radius + B.radius);
  return (dx*dx + dy*dy + dz*dz < rr);
}

void BroadphaseCPU::update(float dt) {
  // Integrating Bodies
  for (auto& b : collisionBodies) {
    b.position += b.velocity * dt;
  }

  // updating bvh entries
  std::vector<int> updatedBodies;
  for (int i = 0; i < (int)collisionBodies.size(); i++) {
    AABB tight = collisionBodies[i].tightAABB();
    AABB stored = getStoredAABB(i);

    if (!containedIn(stored, tight)) {
      AABB newAABB = tight;
      newAABB.expand(expansionAmount);
      tree.remove(i);
      tree.insert(i, newAABB);
      updatedBodies.push_back(i);
    }
  }

  // Find new pairs
  std::vector<Pair> newPairs;
  for (int idx : updatedBodies) {
    AABB aabb = getStoredAABB(idx);
    std::vector<int> overlapping = tree.query(aabb);
    for (int other : overlapping) {
      if (other == idx) continue;
      Pair p = makePair(idx, other);
      if (activePairs.find(p) == activePairs.end()) {
        newPairs.push_back(p);
      }
    }
  }

  for (auto& p : newPairs) {
    activePairs.insert(p);
  }

  // Validate old pairs
  for (auto it = activePairs.begin(); it != activePairs.end();) {
    const Pair& pair = *it;
    bool aUpdated = (std::find(updatedBodies.begin(), updatedBodies.end(), pair.bodyA) != updatedBodies.end());
    bool bUpdated = (std::find(updatedBodies.begin(), updatedBodies.end(), pair.bodyB) != updatedBodies.end());

    if (aUpdated || bUpdated) {
      AABB A = getStoredAABB(pair.bodyA);
      AABB B = getStoredAABB(pair.bodyB);
      if (!A.overlaps(B)) {
        it = activePairs.erase(it);
        continue;
      }
    }
    ++it;
  }

  // Narrowphase
  for (auto it = activePairs.begin(); it != activePairs.end();) {
    const Pair& pair = *it;
    if (!narrowphase(collisionBodies[pair.bodyA], collisionBodies[pair.bodyB])) {
      it = activePairs.erase(it);
    } else {
      ++it;
    }
  }

  // After this, collisions are resolved in resolveCollisions if needed
}

// simple resolve for now
void BroadphaseCPU::resolveCollisions(float dt) {
  for (auto it = activePairs.begin(); it != activePairs.end();) {
    const Pair& pair = *it;
    if (pair.bodyA >= collisionBodies.size() || pair.bodyB >= collisionBodies.size()) {
      it = activePairs.erase(it); // Remove invalid pair and get the next iterator
      continue;
    }

    if (pair.bodyA != pair.bodyB) {
      // Swap velocities correctly
      std::swap(collisionBodies[pair.bodyA], collisionBodies[pair.bodyB]);
    }

    ++it; // Move to the next pair
  }
}
