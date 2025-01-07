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
    float bladeRadius = 0.f;
    float bladePitch = 0.f;
    unsigned int bladeCount = 1;
    // TODO: Move to Environment
    float airDensity = 0.f;
    float discArea = 1.f;
    float liftCoefficient = 0.f;
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