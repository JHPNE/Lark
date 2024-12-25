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
}