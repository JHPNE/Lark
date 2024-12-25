#include "RigidBody.h"

namespace drosim::physics {
  RigidBody::RigidBody() {};

  void RigidBody::AddCollider(const Collider &collider) {
    m_colliders.push_back(collider);

    m_colliders.back().SetRigidBody(this);

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
    m_globalCentroid = m_orientation * m_localCentroid + m_position;
  }

  void RigidBody::UpdatePositionFromGlobalCentroid() {
    m_position = m_orientation * (-m_localCentroid) + m_globalCentroid;
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
    // lin velo
    m_linearVelocity += (m_inverseMass * m_forceAccumulator) * dt;

    // angular velo
    glm::vec3 deltaOmega = m_globalInverseInertiaTensor * (m_torqueAccumulator * dt);
    m_angularVelocity += deltaOmega;

    // reset force/torque
    m_forceAccumulator = glm::vec3(0.0f);
    m_torqueAccumulator = glm::vec3(0.0f);

    // integrate pos
    m_globalCentroid += m_linearVelocity * dt;

    // integrate orientation
    float angularSpeed = glm::length(m_angularVelocity);
    if (angularSpeed > 1e-8f) {
      glm::vec3 axis = m_angularVelocity / angularSpeed;
      float angle = angularSpeed * dt;
      // rotation matrix
      glm::mat4 rot4x4 = glm::rotate(glm::mat4(1.0f), angle, axis);
      glm::mat3 rot3x3 = glm::mat3(rot4x4);

      m_orientation = rot3x3 * m_orientation;
    }

    UpdateOrientation();

    UpdatePositionFromGlobalCentroid();
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