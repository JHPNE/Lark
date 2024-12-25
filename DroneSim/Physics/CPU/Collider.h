#pragma once
#include <glm/glm.hpp>
#include "Shape.h"

namespace drosim::physics {
  struct Ray3;  // Forward declaration
  class RigidBody;
  
  class Collider {
    public:
      Collider() = default;
      Collider(const Shape& shape);

      float GetMass() const { return m_mass; };
      const glm::mat3& GetLocalInertiaTensor() const { return m_localInertiaTensor; };
      const glm::vec3& GetLocalCentroid() const {return m_localCentroid; };

      RigidBody* GetRigidBody() const { return m_owningBody; }
      void SetRigidBody(RigidBody* body) {m_owningBody = body;}

      virtual bool RayCast(const Ray3& ray,
                           float &tOut,
                           glm::vec3 &nOut) const {
        // Default = no intersection
        // or throw an exception if you prefer
        return false;
      }

      virtual glm::vec3 Support(const glm::vec3& direction) const {
        return glm::vec3(0.0f);
      }

    private:
      float m_mass = 0.0f;
      glm::mat3 m_localInertiaTensor = glm::mat3(0.0f);
      glm::vec3 m_localCentroid = glm::vec3(0.0f);
      RigidBody* m_owningBody = nullptr;
  };
}