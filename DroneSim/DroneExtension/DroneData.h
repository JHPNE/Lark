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
    float powerConsumption;
    float mass;
    BodyType type;
    btRigidBody* rigidBody;
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
  };

  struct DroneData {
    DroneType type;
    std::vector<std::unique_ptr<Body>> bodies;
    std::vector<Constraints> constraints;
  };
}