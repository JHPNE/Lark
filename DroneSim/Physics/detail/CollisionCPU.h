#include "Physics/PhysicsStructures.h"
#include "BoundingVolumeHierarchyCPU.h"
#include <unordered_set>

struct CollisionBody {
  glm::vec3 position;
  glm::vec3 velocity;
  float radius;

  AABB tightAABB() const {
    return { {position.x - radius, position.y - radius, position.z - radius},
          {position.x + radius, position.y + radius, position.z + radius}};
  };
};

struct Pair {
  int bodyA, bodyB;

  bool operator==(const Pair& other) const {
    return (bodyA == other.bodyA && bodyB == other.bodyB) ||
           (bodyA == other.bodyB && bodyB == other.bodyA);
  }
};

// basic implementation for cpu at least now
class BroadphaseCPU {
  public:
    BVH tree;
    std::vector<CollisionBody> collisionBodies;
    std::unordered_set<Pair> activePairs;

    float expansionAmount = 0.5f;

    void update(float dt);


  private:
    bool containedIn(const AABB& container, const AABB& inner);

    AABB getStoredAABB(int bodyIndex);

    Pair makePair(int a, int b);

    bool narrowphase(const CollisionBody& A, const CollisionBody& B);
};
