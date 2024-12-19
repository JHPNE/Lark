#pragma once
#include "PhysicsBackend.h"

class CpuPhysicsBackend : public PhysicsBackend {
  public:
    CpuPhysicsBackend(RigidBodyArrays &rb) : rbData(rb) {};

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
  private:
    RigidBodyArrays &rbData;
};