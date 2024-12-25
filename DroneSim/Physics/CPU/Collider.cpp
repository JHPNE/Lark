#include "Collider.h"

namespace drosim::physics {
  Collider::Collider() {
    m_aabb = std::make_unique<AABB>();
  }

  Collider::Collider(const Shape& shape) : m_shape(&shape) {
    m_mass = shape.ComputeMass();
    m_localInertiaTensor = shape.ComputeLocalInertiaTensor();
    m_localCentroid = shape.ComputeLocalCentroid();
    m_aabb = std::make_unique<AABB>();
  }

  Collider::Collider(const Collider& other) :
      m_mass(other.m_mass),
      m_localInertiaTensor(other.m_localInertiaTensor),
      m_localCentroid(other.m_localCentroid),
      m_shape(other.m_shape),
      m_owningBody(other.m_owningBody) {
    m_aabb = std::make_unique<AABB>();
  }

  Collider& Collider::operator=(const Collider& other) {
    if (this != &other) {
      m_mass = other.m_mass;
      m_localInertiaTensor = other.m_localInertiaTensor;
      m_localCentroid = other.m_localCentroid;
      m_shape = other.m_shape;
      m_owningBody = other.m_owningBody;
      m_aabb = std::make_unique<AABB>();
    }
    return *this;
  }

  Collider::~Collider() = default;
}