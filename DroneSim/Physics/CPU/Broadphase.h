#pragma once
#include "PhysicsStructures.h"
#include <list>
#include <vector>

namespace drosim::physics {
  typedef std::pair<Collider*, Collider*> ColliderPair;
  typedef std::list<ColliderPair> ColliderPairList;

   class Broadphase {
    public:
      // adds a new AABB
      virtual void Add(AABB *aabb) = 0;
      virtual void Remove(AABB *aabb) = 0;

      virtual void Update() = 0;

      virtual const ColliderPairList &ComputePairs() = 0;

      virtual Collider *Pick(const glm::vec3 &point) const = 0;

      typedef std::vector<Collider*> ColliderList;
      virtual void Query(const AABB &aabb, ColliderList &output) const = 0;
      virtual RayCastResult RayCast(const Ray3 &ray) const = 0;
  };

};