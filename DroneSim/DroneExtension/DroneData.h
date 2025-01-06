#pragma once
#include <vector>
#include <memory>
#include <string>
#include <tuple>
#include "../Common/Id.h"
#include <glm/glm.hpp>
#include <btBulletDynamicsCommon.h>


namespace lark::drone_data {

  enum class DroneType {
    MULTIROTOR,
    FIXED_WING,
    HYBRID,
  };

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
  };

  struct Constraints {
     std::pair<Body, Body> connection;
  };

  struct FuselageBody : public Body {
    BodyType type = BodyType::FUSELAGE;
  };

  struct RotorBody : public Body{
    BodyType type = BodyType::ROTOR;
  };

  struct WingBody : public Body {
    BodyType type = BodyType::WING;
  };

  struct BatteryBody : public Body {
    BodyType type = BodyType::BATTERY;
    float batteryCapacity = 0.f;
    float batteryVoltage = 0.f;
    float selfDischargeRate = 0.f;
    float internalResistance = 0.f;
    float cRating = 0.f;
  };

  struct DroneData {
    DroneType type;
    std::vector<std::unique_ptr<Body>> bodies;
    std::vector<Constraints> constraints;
  };
}