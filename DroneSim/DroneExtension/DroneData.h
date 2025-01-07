#pragma once
#include <vector>
#include <memory>
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

  struct Body {
    virtual ~Body() = default;
    float powerConsumption = 0.f;
    float mass = 0.f;
    BodyType type = BodyType::FUSELAGE;
    btVector3 position = btVector3(0.f, 0.f, 0.f);
    btRigidBody* rigidBody = nullptr;
    btTriangleMesh* meshInterface{nullptr};
    // TODO constraints added here if not found maybe detect closest?
    util::vector<Body> connections{};
  };

  struct FuselageBody : Body {
    BodyType type = BodyType::FUSELAGE;
  };

  struct RotorBody : Body{
    BodyType type = BodyType::ROTOR;
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