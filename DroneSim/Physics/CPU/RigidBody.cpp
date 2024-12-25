#include "RigidBody.h"

#include "Colliders/BoxCollider.h"

namespace drosim::physics {
  RigidBody::RigidBody() {};

void RigidBody::AddCollider(std::unique_ptr<Collider> collider) {
  // Let the collider know which body it belongs to
  collider->SetRigidBody(this);

  // If it's a BoxCollider, we can do an initial AABB update, for debugging:
  if (auto* boxCollider = dynamic_cast<BoxCollider*>(collider.get())) {
    boxCollider->UpdateAABBBounds();
    AABB* aabb = boxCollider->GetAABB();
    if (aabb) {
      std::cout << "Initial AABB bounds for body at Y=" << GetPosition().y
                << ": [" << aabb->minPoint.y << ", " << aabb->maxPoint.y << "]\n";
    }
  }

  // Add it to the internal list
  m_colliders.push_back(std::move(collider));

  // Recompute mass properties
  UpdateMassProperties();
}

void RigidBody::UpdateMassProperties() {
  m_localCentroid = glm::vec3(0.0f);

  // Sum up mass from each collider
  for (auto &cPtr : m_colliders) {
    Collider& c = *cPtr;
    m_mass += c.GetMass();
    m_localCentroid += c.GetMass() * c.GetLocalCentroid();
  }

  if (m_mass > 1e-8f) {
    m_inverseMass = 1.0f / m_mass;
    m_localCentroid *= m_inverseMass;
  } else {
    m_inverseMass = 0.0f;
  }

  glm::mat3 localInertia(0.0f);
  for (auto &cPtr : m_colliders) {
    Collider& c = *cPtr;
    const glm::vec3 r = m_localCentroid - c.GetLocalCentroid();
    float rDotR = glm::dot(r, r);
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

  // Update position
  m_position += m_linearVelocity * dt;

  // Update orientation if there's angular velocity
  float angularSpeed = glm::length(m_angularVelocity);
  if (angularSpeed > 1e-8f) {
    glm::vec3 axis = m_angularVelocity / angularSpeed;
    float angle = angularSpeed * dt;
    glm::mat4 rot4x4 = glm::rotate(glm::mat4(1.0f), angle, axis);
    glm::mat3 rot3x3 = glm::mat3(rot4x4);
    m_orientation = rot3x3 * m_orientation;
    UpdateOrientation();
  }

  // Update global centroid from new position
  UpdateGlobalCentroidFromPosition();

  // Debug position change
  std::cout << "Body moved from Y=" << initialPos.y
            << " to Y=" << m_position.y << "\n";
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