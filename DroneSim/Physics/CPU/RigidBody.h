#pragma once
#include "Collider.h"
#include "Physics/PhysicsStructures.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>

#include <memory>

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

      void SetPosition(const glm::vec3 &pos) { m_position = pos;};
      glm::vec3 GetPosition() { return m_position; };

      void SetVelocity(const glm::vec3 &vel) { m_linearVelocity = vel;};
      glm::vec3 GetVelocity() { return m_linearVelocity;};

      void SetAngularVelocity(const glm::vec3 &vel) { m_angularVelocity = vel;};
      glm::vec3 GetAngularVelocity() { return m_angularVelocity;};

      void SetMass(float mass) { m_mass = mass;}
      float GetMass() const { return m_mass; }

      float GetInverseMass() const { return m_inverseMass; }

      glm::mat3 GetLocalInverseInertiaTensor() const { return m_localInverseInertiaTensor; }

      const std::vector<Collider>& GetColliders() const { return m_colliders; }
      std::vector<Collider>& GetColliders() { return m_colliders; }

      const glm::mat3& GetOrientation() const { return m_orientation; }

      // Local->Global / Global->Local conversions
      glm::vec3 LocalToGlobal(const glm::vec3 &p) const;
      glm::vec3 GlobalToLocal(const glm::vec3 &p) const;
      glm::vec3 LocalToGlobalVec(const glm::vec3 &v) const;
      glm::vec3 GlobalToLocalVec(const glm::vec3 &v) const;

      void ApplyImpulse(const glm::vec3& impulse, const glm::vec3& worldPoint) {
        if (m_inverseMass == 0.0f) return; // Static bodies don't respond to impulses

        // Linear impulse
        m_linearVelocity += impulse * m_inverseMass;

        // Angular impulse
        glm::vec3 relativePoint = worldPoint - m_position;
        glm::vec3 angularImpulse = glm::cross(relativePoint, impulse);
        m_angularVelocity += m_globalInverseInertiaTensor * angularImpulse;
      }

      // Utility methods for impulse application
      void ApplyLinearImpulse(const glm::vec3& impulse) {
        if (m_inverseMass == 0.0f) return;
        m_linearVelocity += impulse * m_inverseMass;
      }

      void ApplyAngularImpulse(const glm::vec3& impulse) {
        if (m_inverseMass == 0.0f) return;
        m_angularVelocity += m_globalInverseInertiaTensor * impulse;
      }

      // Add these accessor methods if not already present:
      const glm::mat3& GetGlobalInverseInertiaTensor() const {
        return m_globalInverseInertiaTensor;
      }

      float GetRestitution() const { return m_restitution; }
      void SetRestitution(float restitution) { m_restitution = restitution; }

      float GetFriction() const { return m_friction; }
      void SetFriction(float friction) { m_friction = friction; }

    private:
      float m_mass              = 0.0f;
      float m_inverseMass       = 0.0f;
      float m_restitution = 0.2f;  // Coefficient of restitution
      float m_friction = 0.7f;     // Coefficient of friction

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

      std::vector<std::unique_ptr<AABB>> m_aabbs;

      // Helper to recalc global inverse inertia
      void UpdateGlobalInverseInertia();
      void UpdateMassProperties();
  };
}