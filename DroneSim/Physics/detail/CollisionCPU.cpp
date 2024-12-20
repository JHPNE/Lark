#include "CollisionCPU.h"

void BroadphaseCPU::update(float dt) {
  // Integrating Bodies
  for (auto& b : collisionBodies) {
    b.position += b.velocity * dt;
  }

  // updating bvh entries
  std::vector<int> updatedBodies;
  for (int i = 0; i < (int)collisionBodies.size(); i++) {
    AABB tight = collisionBodies[i].tightAABB();
    AABB stored = getStoredAABB(i); // AABB in the BVH

    if (!containedIn(stored, tight)) {
      AABB newAABB = tight;
      newAABB.expand(expansionAmount);
      tree.remove(i);
      tree.insert(i, newAABB);
      updatedBodies.push_back(i);
    }
  }

  // 3. Find new pairs from updated bodies
  std::vector<Pair> newPairs;
  for (int idx : updatedBodies) {
    AABB aabb = getStoredAABB(idx);
    std::vector<int> overlapping = tree.query(aabb);
    for (int other : overlapping) {
      if (other == idx) continue;

      Pair p = makePair(idx, other);
      if (activePairs.find(p) == activePairs.end()) {
        // New candidate
        newPairs.push_back(p);
      }
    }
  }

  // Insert new pairs into activePairs
  for (auto& p : newPairs) {
    activePairs.insert(p);
  }

  // 4. Validate old pairs
  // For performance, you can skip pairs if neither object in the pair was updated.
  // Only re-check bounding volumes for pairs involving updated bodies.
  for (auto it = activePairs.begin(); it != activePairs.end(); ) {
    const Pair& pair = *it;
    bool aUpdated = std::find(updatedBodies.begin(), updatedBodies.end(), pair.bodyA) != updatedBodies.end();
    bool bUpdated = std::find(updatedBodies.begin(), updatedBodies.end(), pair.bodyB) != updatedBodies.end();

    if (aUpdated || bUpdated) {
      // re-check overlap
      AABB A = getStoredAABB(pair.bodyA);
      AABB B = getStoredAABB(pair.bodyB);
      if (!A.overlaps(B)) {
        // no longer overlapping at the broadphase level
        it = activePairs.erase(it);
        continue;
      }
    }

    ++it;
  }

  // 5. Narrowphase
  for (auto it = activePairs.begin(); it != activePairs.end(); ) {
    const Pair& pair = *it;
    if (!narrowphase(collisionBodies[pair.bodyA], collisionBodies[pair.bodyB])) {
      // no collision found
      it = activePairs.erase(it);
    } else {
      ++it;
    }
  }
}

bool BroadphaseCPU::containedIn(const AABB& container, const AABB& inner) {
  return (container.min.x <= inner.min.x && container.min.y <= inner.min.y &&
          container.max.x >= inner.max.x && container.max.y >= inner.max.y);
}

AABB BroadphaseCPU::getStoredAABB(int bodyIndex) {
  // Retrieve the AABB from the BVH node for this body
  // Implementation dependent on your BVH data structure
};

Pair BroadphaseCPU::makePair(int a, int b) {
  // To maintain a consistent ordering of body indices in the pair
  return (a < b) ? Pair{a,b} : Pair{b,a};
};

bool BroadphaseCPU::narrowphase(const CollisionBody& A, const CollisionBody& B) {
  // Simple circle-circle test: they collide if distance < sum of radii
  float dx = A.position.x - B.position.x;
  float dy = A.position.y - B.position.y;
  float rr = (A.radius + B.radius)*(A.radius + B.radius);
  return (dx*dx + dy*dy < rr);
};