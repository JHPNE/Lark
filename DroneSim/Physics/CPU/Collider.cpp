#include "Collider.h"

namespace drosim::physics {
  Collider::Collider(const Shape &shape) {
    m_mass = shape.ComputeMass();
    m_localInertiaTensor = shape.ComputeLocalInertiaTensor();
    m_localCentroid = shape.ComputeLocalCentroid();
  }
}