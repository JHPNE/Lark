#pragma once
#include "PhysicsStructures.h"
#include "Shape.h"
#include <glm/glm.hpp>
#include <memory>

namespace drosim::physics {

class RigidBody; // Forward declaration

class Collider {
public:
  // Default constructor
  Collider();
  // Construct from a shape (BoxShape, SphereShape, etc.)
  explicit Collider(const Shape& shape);

  // Delete copy constructor/assignment to prevent accidental copying
  Collider(const Collider&) = delete;
  Collider& operator=(const Collider&) = delete;

  // Virtual destructor
  ~Collider() = default;

  float GetMass() const { return m_mass; }
  const glm::mat3& GetLocalInertiaTensor() const { return m_localInertiaTensor; }
  const glm::vec3& GetLocalCentroid() const { return m_localCentroid; }
  virtual const Shape& GetShape() const = 0;

  RigidBody * GetRigidBody() const { return m_owningBody; }
  void SetRigidBody(RigidBody* body) { m_owningBody = body; }

  AABB* GetAABB() const { return m_aabb.get(); }

  // Optional raycast and support functions
  virtual bool RayCast(const Ray3& /*ray*/, float &/*tOut*/, glm::vec3 &/*nOut*/) const {
    return false;
  }

  virtual glm::vec3 Support(const glm::vec3& /*direction*/) const {
    return glm::vec3(0.0f);
  }

  virtual void UpdateAABBBounds() {};

protected:
  float m_mass             = 0.0f;
  glm::mat3 m_localInertiaTensor = glm::mat3(0.0f);
  glm::vec3 m_localCentroid = glm::vec3(0.0f);
  const Shape* m_shape     = nullptr;
  RigidBody* m_owningBody  = nullptr;

  // Each Collider owns its AABB
  std::unique_ptr<AABB> m_aabb;
};

} // namespace drosim::physics
