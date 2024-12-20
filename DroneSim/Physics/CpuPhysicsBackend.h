#pragma once
#include "PhysicsBackend.h"
#include "detail/CollisionCPU.h"

class CpuPhysicsBackend : public PhysicsBackend {
  public:
    CpuPhysicsBackend(RigidBodyArrays &rb) : rbData(rb) {
      size_t count = rbData.positions.size();
      broadphase.collisionBodies.resize(count);

      for (size_t i = 0; i < count; i++) {
        CollisionBody body;
        body.position = rbData.positions[i];
        body.velocity = rbData.linearVelocities[i];

        // If radius isn't stored directly in rbData, you need a way to determine it.
        // For now, assume a radius from rbData or a default value:
        body.radius = 1.0f; // or from rbData if available

        broadphase.collisionBodies[i] = body;
      }

      // Optionally build an initial BVH. If your BVH builds as you go, skip this.
      for (int i = 0; i < (int)broadphase.collisionBodies.size(); i++) {
        AABB initialAABB = broadphase.collisionBodies[i].tightAABB();
        initialAABB.expand(broadphase.expansionAmount);
        broadphase.tree.insert(i, initialAABB);
      }
    };

    // See PhysicsStructures for diagonal vectors
    void updateRigidBodies(size_t count, float dt) override {
      for (size_t i = 0; i < count; i++) {
        float mass = rbData.massData[i].x;
        float invMass = rbData.massData[i].y;

        if (invMass > 0.0f) {
          glm::vec3 inertia(
              rbData.massData[i].z,
              rbData.massData[i].w,
              rbData.inertiaData[i].x
          );

          glm::vec3 invInertia(
              rbData.inertiaData[i].y,
              rbData.inertiaData[i].z,
              rbData.inertiaData[i].w
          );

          // Updating linear velocity
          rbData.linearVelocities[i] += GetEnvironment().Gravity * dt;

          // Update angular velocity
          // Torque
          // w += invInertia * T * dt

          // Updating Positions
          rbData.positions[i] += rbData.linearVelocities[i] * dt;

          // Updating orientation
          glm::quat q = rbData.orientations[i];
          glm::vec3 wv = rbData.angularVelocities[i];
          glm::quat wq(0.0f, wv.x, wv.y, wv.z);
          glm::quat qDot = 0.5f * wq * q;
          glm::quat newQ = q + qDot * dt;
          rbData.orientations[i] = glm::normalize(newQ);
        }
      }
    }

    void detectCollisions(float dt) override {
      // Here we can call the broadphase to update and find new collisions
      broadphase.update(dt);

      // After update, broadphase.activePairs contains potential collision pairs
      // and broadphase.contacts can be filled in the future if you add narrowphase details.
    }

    void resolveCollisions(float dt) override {
      // Apply responses based on broadphase (and potentially narrowphase) results
      // If you've implemented broadphase.resolveCollisions, call it here
      broadphase.resolveCollisions(dt);

      // The resolveCollisions function can adjust velocities/positions of objects
      // in broadphase.collisionBodies, which you then copy back to rbData if needed
      for (size_t i = 0; i < rbData.positions.size(); i++) {
        rbData.positions[i] = broadphase.collisionBodies[i].position;
        rbData.linearVelocities[i] = broadphase.collisionBodies[i].velocity;
      }
    }

  private:
    RigidBodyArrays &rbData;
    BroadphaseCPU broadphase;
};