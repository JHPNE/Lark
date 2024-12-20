#include "Physics/PhysicsStructures.h"

class BVH {
  public:
    void remove(int index);
    void insert(int index, AABB aabb);
};