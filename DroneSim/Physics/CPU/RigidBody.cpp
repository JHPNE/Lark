#include "RigidBody.h"

#include "Colliders/BoxCollider.h"

namespace drosim::physics {
  RigidBody::RigidBody() {};

  void RigidBody::AddCollider(const Collider &collider) {
    m_colliders.push_back(collider);
    m_colliders.back().SetRigidBody(this);

    // Ensure AABB is initialized and connected to collider
    AABB* aabb = m_colliders.back().GetAABB();
    if (aabb) {
      aabb->userData = &m_colliders.back();

      // Initialize AABB bounds based on collider type and body position
      const BoxCollider* boxCollider = dynamic_cast<const BoxCollider*>(&collider);
      if (boxCollider) {
        glm::vec3 halfExtents = boxCollider->m_shape.m_halfExtents;
        glm::vec3 pos = GetPosition();
        aabb->minPoint = pos - halfExtents;
        aabb->maxPoint = pos + halfExtents;
      }
    }

    // Update mass properties
    UpdateMassProperties();
  }

  void RigidBody::UpdateMassProperties() {
    m_mass = 0.0f;
    m_localCentroid = glm::vec3(0.0f);

    for (auto &c : m_colliders) {
      m_mass += c.GetMass();
      m_localCentroid += c.GetMass() * c.GetLocalCentroid();
    }

    if (m_mass > 1e-8f){
      m_inverseMass = 1.0f / m_mass;
      m_localCentroid *= m_inverseMass;
    } else {
      m_inverseMass = 0.0f;
    }

    glm::mat3 localInertia(0.0f);
    for (auto &c : m_colliders) {
      const glm::vec3 r = m_localCentroid - c.GetLocalCentroid();
      const float rDotR = glm::dot(r, r);
      glm::mat3 rOutR = glm::mat3(
          r.x*r.x, r.x*r.y, r.x*r.z,
          r.y*r.x, r.y*r.y, r.y*r.z,
          r.z*r.x, r.z*r.y, r.z*r.z
      );

      localInertia += c.GetLocalInertiaTensor() + c.GetMass() * (rDotR * glm::mat3(1.0f) - rOutR);
    }

    m_localInverseInertiaTensor = glm::inverse(localInertia);

    UpdateGlobalInverseInertia();
  }

  void RigidBody::ApplyForce(const glm::vec3 &force, const glm::vec3 &worldPoint) {
    m_forceAccumulator += force;
    glm::vec3 leverArm = worldPoint - m_globalCentroid;
    m_torqueAccumulator += glm::cross(leverArm, force);
  }

  void RigidBody::UpdateGlobalCentroidFromPosition() {
    m_globalCentroid = m_position + m_orientation * m_localCentroid;
  }

  void RigidBody::UpdatePositionFromGlobalCentroid() {
    m_position = m_globalCentroid - m_orientation * m_localCentroid;
  }

  void RigidBody::UpdateOrientation() {
    glm::quat q = glm::quat_cast(m_orientation);

    q = glm::normalize(q);

    m_orientation = glm::mat3_cast(q);

    m_inverseOrientation = glm::transpose(m_orientation);

    UpdateGlobalInverseInertia();
  }

  void RigidBody::UpdateGlobalInverseInertia() {
    // globalInverseInertiaTensor = R * localInverseInertia * R^T
    // We have orientation (R) and inverse orientation (R^T):
    m_globalInverseInertiaTensor = m_orientation
                                 * m_localInverseInertiaTensor
                                 * m_inverseOrientation;
  }

  void RigidBody::Integrate(float dt) {
    if (m_inverseMass == 0.0f) return;

    // Store initial state for debug
    glm::vec3 initialPos = m_position;
    glm::vec3 initialVel = m_linearVelocity;

    // Update linear velocity from forces
    glm::vec3 acceleration = m_forceAccumulator * m_inverseMass;
    m_linearVelocity += acceleration * dt;

    // Update angular velocity from torques
    glm::vec3 deltaOmega = m_globalInverseInertiaTensor * (m_torqueAccumulator * dt);
    m_angularVelocity += deltaOmega;

    // Reset force/torque accumulators
    m_forceAccumulator = glm::vec3(0.0f);
    m_torqueAccumulator = glm::vec3(0.0f);

    // Direct position integration instead of going through centroid
    m_position += m_linearVelocity * dt;

    // Update orientation if there's angular velocity
    float angularSpeed = glm::length(m_angularVelocity);
    if (angularSpeed > 1e-8f) {
      glm::vec3 axis = m_angularVelocity / angularSpeed;
      float angle = angularSpeed * dt;
      glm::mat4 rot4x4 = glm::rotate(glm::mat4(1.0f), angle, axis);
      glm::mat3 rot3x3 = glm::mat3(rot4x4);
      m_orientation = rot3x3 * m_orientation;
    }

    // Update orientation-derived quantities
    UpdateOrientation();

    // Update global centroid from new position
    UpdateGlobalCentroidFromPosition();
  }

  glm::vec3 RigidBody::LocalToGlobal(const glm::vec3 &p) const {
    return m_orientation * p + m_position;
  }

  glm::vec3 RigidBody::GlobalToLocal(const glm::vec3 &p) const {
    return m_inverseOrientation * (p - m_position);
  }

  glm::vec3 RigidBody::LocalToGlobalVec(const glm::vec3 &v) const {
    return m_orientation * v;
  }

  glm::vec3 RigidBody::GlobalToLocalVec(const glm::vec3 &v) const {
    return m_inverseOrientation * v;
  }

}