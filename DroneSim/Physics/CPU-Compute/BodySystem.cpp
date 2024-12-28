#include "BodySystem.h"
#include <omp.h>
#include <algorithm>
#include <cmath>

namespace drosim::physics::cpu {
  namespace {
    bool isValidId(uint32_t id) {
      return id != UINT32_MAX;
    }
  }

  uint32_t CreateBody(PhysicsWorld& world, const glm::vec3& pos, float mass) {
    uint32_t idx = static_cast<uint32_t>(world.bodyPool.Allocate());
    assert(isValidId(idx));

    auto& body = world.bodyPool[idx];
    body.motion.position       = pos;
    body.motion.velocity       = glm::vec3(0.f);
    body.motion.angularVelocity= glm::vec3(0.f);
    body.motion.orientation    = glm::quat(1.f, 0.f, 0.f, 0.f); // identity quaternion

    body.inertia.mass = mass;
    body.inertia.invMass = (mass > 0.f) ? 1.f / mass : 0.f;
    body.inertia.localInertia = glm::mat3(1.f);
    body.inertia.invLocalInertia = glm::mat3(1.f);
    body.inertia.globalInvInertia = glm::mat3(1.f);

    body.forces.force  = glm::vec3(0.f);
    body.forces.torque = glm::vec3(0.f);

    body.material.friction    = 0.7f;
    body.material.restitution = 0.2f;

    body.flags.active   = (mass > 0.f);
    body.flags.isStatic = (mass == 0.f);

    return idx;
  }

  glm::vec3 GetBodyPosition(PhysicsWorld& world, uint32_t id) {
    return world.bodyPool[id].motion.position;
  }

  void IntegrateForces(PhysicsWorld& world, float dt) {
    #pragma omp parallel for
    for (size_t i = 0; i < world.bodyPool.Size(); ++i) {
      auto& body = world.bodyPool[i];
      if (!body.flags.active || body.flags.isStatic) continue;

      // accumulate force
      glm::vec3 totalForce = body.forces.force + world.gravity * body.inertia.mass;
      body.motion.velocity += totalForce * body.inertia.invMass * dt;

      // accumulate torque
      glm::vec3 angAcc = body.inertia.globalInvInertia * body.forces.torque;
      body.motion.angularVelocity += angAcc * dt;
    }
  }

  void IntegrateVelocities(PhysicsWorld& world, float dt) {
    #pragma omp parallel for
    for (size_t i = 0; i < world.bodyPool.Size(); ++i) {
      auto& body = world.bodyPool[i];
      if (!body.flags.active || body.flags.isStatic) continue;

      // linear
      body.motion.position += body.motion.velocity * dt;

      // angular
      glm::vec3 omega = body.motion.angularVelocity;
      float angle = glm::length(omega);

      if (angle > 1e-6f) {
        glm::vec3 axis = omega / angle;
        float halfAngle = angle * dt * 0.5f;
        float s = std::sin(halfAngle);
        glm::quat dq(std::cos(halfAngle), axis.x*s, axis.y*s, axis.z*s);
        // update orientation
        body.motion.orientation = dq * body.motion.orientation;
      }

      // Normalize to avoid drift
      body.motion.orientation = glm::normalize(body.motion.orientation);

      // Recompute global inverse inertia
      glm::mat3 R = glm::mat3_cast(body.motion.orientation);
      body.inertia.globalInvInertia = R * body.inertia.invLocalInertia * glm::transpose(R);
    }
  }

  void UpdateSleeping(PhysicsWorld& world) {
    for (size_t i = 0; i < world.bodyPool.Size(); ++i) {
      auto& body = world.bodyPool[i];
      if (body.flags.isStatic) continue;

      float linSpeed = glm::length(body.motion.velocity);
      float angSpeed = glm::length(body.motion.angularVelocity);

      if (linSpeed < world.sleepLinThreshold && angSpeed < world.sleepAngThreshold) {
        body.flags.active = false;
        body.motion.velocity = glm::vec3(0.f);
        body.motion.angularVelocity = glm::vec3(0.f);
      } else {
        body.flags.active = true;
      }
    }
  }

  void ClearForces(PhysicsWorld& world) {
    #pragma omp parallel for
    for (size_t i = 0; i < world.bodyPool.Size(); ++i) {
      world.bodyPool[i].forces.force = glm::vec3(0.f);
      world.bodyPool[i].forces.torque = glm::vec3(0.f);
    }
  }
}
