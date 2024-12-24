#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include "Collider.h"

namespace drosim::physics {

  class RigidBody {
    public:
      RigidBody();
      ~RigidBody() = default;

      void AddCollider(const Collider& collider);

      void ApplyForce(const glm::vec3 &force, const glm::vec3 &worldPoint);

      void UpdateGlobalCentroidFromPosition();
      void UpdatePositionFromGlobalCentroid();

      void UpdateOrientation();

      void Integrate(float dt);

      inline void SetPosition(const glm::vec3 &pos) { m_position = pos;};
      inline glm::vec3 GetPosition() { return m_position; };

      inline void SetVelocity(const glm::vec3 &vel) { m_linearVelocity = vel;};
      inline glm::vec3 GetVelocity() { return m_linearVelocity;};

      inline void SetAngularVelocity(const glm::vec3 &vel) { m_angularVelocity = vel;};
      inline glm::vec3 GetAngularVelocity() { return m_angularVelocity;};

      inline float GetMass() const { return m_mass; }
      inline float GetInverseMass() const { return m_inverseMass; }

      // Local->Global / Global->Local conversions
      glm::vec3 LocalToGlobal(const glm::vec3 &p) const;
      glm::vec3 GlobalToLocal(const glm::vec3 &p) const;
      glm::vec3 LocalToGlobalVec(const glm::vec3 &v) const;
      glm::vec3 GlobalToLocalVec(const glm::vec3 &v) const;
    private:
      float m_mass              = 0.0f;
      float m_inverseMass       = 0.0f;

      glm::mat3 m_localInverseInertiaTensor  = glm::mat3(0.0f);
      glm::mat3 m_globalInverseInertiaTensor = glm::mat3(0.0f);

      // --- Centroids ---
      glm::vec3 m_globalCentroid = glm::vec3(0.0f);
      glm::vec3 m_localCentroid  = glm::vec3(0.0f);

      // --- Position / Orientation ---
      glm::vec3 m_position        = glm::vec3(0.0f);
      glm::mat3 m_orientation     = glm::mat3(1.0f);  // 3×3 rotation matrix
      glm::mat3 m_inverseOrientation = glm::mat3(1.0f);

      // --- Velocities ---
      glm::vec3 m_linearVelocity  = glm::vec3(0.0f);
      glm::vec3 m_angularVelocity = glm::vec3(0.0f);

      // --- Force / Torque accumulators ---
      glm::vec3 m_forceAccumulator  = glm::vec3(0.0f);
      glm::vec3 m_torqueAccumulator = glm::vec3(0.0f);

      // --- Colliders (each body can contain multiple colliders) ---
      std::vector<Collider> m_colliders;

      // Helper to recalc global inverse inertia
      void UpdateGlobalInverseInertia();
  };
}