#pragma once
#include <vector>
#include <memory>
#include <map>
#include <string>
#include <tuple>
#include "../Common/Id.h"
#include <glm/glm.hpp>
#include <btBulletDynamicsCommon.h>

namespace lark::drone_data {

  enum class BodyType {
    FUSELAGE,
    ROTOR,
    WING,
    BATTERY
  };

  struct ComponentShape {
    enum class ShapeTypes {
       BOX,
       CYLINDER,
       SPHERE,
       CAPSULE,
       CONVEX_HULL
    };

    ShapeTypes type;
    glm::vec3 dimensions;

    btTriangleMesh* mesh;

    btCollisionShape* create_bullet_shape() const {
      switch(type) {
        case ShapeTypes::BOX:
          return new btBoxShape(btVector3(dimensions.x, dimensions.y, dimensions.z));
        case ShapeTypes::CYLINDER:
          return new btCylinderShape(btVector3(dimensions.x, dimensions.y, dimensions.z));
        case ShapeTypes::SPHERE:
          return new btSphereShape(dimensions.x);  // Only uses x component as radius
        case ShapeTypes::CAPSULE:
          return new btCapsuleShape(dimensions.x, dimensions.y * 2.0f);  // radius, height
        case ShapeTypes::CONVEX_HULL:
          if(mesh) {
            return new btConvexTriangleMeshShape(mesh);
          }
        return nullptr;
      }
      return nullptr;
    }
  };

  struct Body {
    virtual ~Body() = default;
    float powerConsumption = 0.f;
    float mass = 0.f;
    BodyType type = BodyType::FUSELAGE;
    glm::mat4 transform;
    btRigidBody* rigidBody = nullptr;
    ComponentShape shape;
  };

  struct FuselageBody : Body {
    BodyType type = BodyType::FUSELAGE;
  };

  struct RotorBody : Body{
    BodyType type = BodyType::ROTOR;
    float bladeRadius = 0.f;
    float bladePitch = 0.f;
    unsigned int bladeCount = 1;
    float discArea = 1.f;
    float currentRPM = 0.f;
    // TODO: Visually set origin as rotation achses maybe offer user the ability to do ithimself
    btVector3 rotorNormal{0, 0, 0};
  };

  struct WingBody : Body {
    BodyType type = BodyType::WING;
  };

  struct BatteryBody : Body {
    BodyType type = BodyType::BATTERY;
    float batteryCapacity = 0.f;
    float batteryVoltage = 0.f;
    float selfDischargeRate = 0.f;
    float internalResistance = 0.f;
    float cRating = 0.f;
  };

  struct EmptyBody : Body {
  };
}