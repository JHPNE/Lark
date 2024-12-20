#include "Physics/PhysicsStructures.h"
#include "BVHCPU.h"
#include <unordered_set>
#include <algorithm>

struct CollisionBody {
  glm::vec3 position;
  glm::vec3 velocity;
  float radius;

  AABB tightAABB() const {
    return { {position.x - radius, position.y - radius, position.z - radius},
             {position.x + radius, position.y + radius, position.z + radius} };
  };
};

// Define a hash for Pair
struct Pair {
  int bodyA, bodyB;

  bool operator==(const Pair& other) const {
    return (bodyA == other.bodyA && bodyB == other.bodyB) ||
           (bodyA == other.bodyB && bodyB == other.bodyA);
  }
};

struct PairHash {
  std::size_t operator()(const Pair &p) const {
    int a = (p.bodyA < p.bodyB) ? p.bodyA : p.bodyB;
    int b = (p.bodyA < p.bodyB) ? p.bodyB : p.bodyA;
    std::hash<int> h;
    return (static_cast<std::size_t>(h(a)) << 1) ^ static_cast<std::size_t>(h(b));
  }
};

struct Contact {
  int bodyA, bodyB;
  glm::vec3 point;
  glm::vec3 normal;
  float penetration;
};

// basic implementation for cpu at least now
class BroadphaseCPU {
  public:
    BVH tree;
    std::vector<CollisionBody> collisionBodies;
    std::unordered_set<Pair, PairHash> activePairs; // use PairHash here
    std::vector<Contact> contacts;

    float expansionAmount = 0.5f;
    float restitution = 0.5f;
    float friction = 0.2f;

    void update(float dt);
    void resolveCollisions(float dt);

  private:
    bool containedIn(const AABB& container, const AABB& inner);

    AABB getStoredAABB(int bodyIndex);

    Pair makePair(int a, int b);

    bool narrowphase(const CollisionBody& A, const CollisionBody& B);

    void resolveContact(const Contact& contact, float dt);
};
