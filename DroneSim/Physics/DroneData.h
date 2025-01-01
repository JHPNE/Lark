#pragma once
#include <vector>
#include <memory>
#include <string>
#include <string>
#include <glm/glm.hpp>
#include <btBulletDynamicsCommon.h>

enum class DroneType {
  MULTIROTOR,
  FIXED_WING,
  HYBRID,
};

struct DroneBody{
   // Bodies
  btRigidBody* fuselageBody; // primary rigid body for the fuselage
  std::vector<btRigidBody*> childBodies; // for multirotors/hybrids
  std::vector<btTypedConstraint*> constraints; // folding arms, tilt rotors etc

  DroneBody() : fuselageBody(nullptr) {}
};

struct DroneAeroDynamics {
  // Aerodynamic parameters (meters)
  float wingArea;
  float wingSpan;
  float cLift;
  float cDrag;

  DroneAeroDynamics() :wingArea(0.0f),wingSpan(0.0f),cLift(0.5f),cDrag(0.1f) {};
};

struct DroneRotors {
   // Rotor Parameters
  uint32_t rotorCount;
  float rotorMaxThrust; // N (per rotor or total)
  std::vector<float> rotorThrottle; // 0..1 for each rotor

  DroneRotors() : rotorCount(4), rotorMaxThrust(40.0f) {};
};

struct DroneControlSystem {
  glm::vec3 altitude; // split in altKP altKI altKD
  glm::vec3 pitch; // same as above
  glm::vec3 roll;
  glm::vec3 yaw;

  DroneControlSystem() : altitude(0.8f, 0.0f, 0.0f), pitch(0.1f, 0.0f, 0.0f), roll(0.1f, 0.0f, 0.0f), yaw(0.1f, 0.0f, 0.0f) {};
};

struct DroneControlIntegrators {
  // Control integrators
  float altIntegral;
  float pitchIntegral;
  float rollIntegral;
  float yawIntegral;

  DroneControlIntegrators() : altIntegral(0), pitchIntegral(0), rollIntegral(0), yawIntegral(0.0f) {};
};

struct DroneSensors {
  // Sensor readings
  float sensedAltitude;
  float sensedPitch;
  float sensedRoll;
  float sensedYaw;

  DroneSensors() : sensedAltitude(0.f), sensedPitch(0.f), sensedRoll(0.f), sensedYaw(0.0f) {};
};

struct DroneBattery {
  // Battery
  float batteryCapacity;
  float batteryLevel;
  float powerConsumption;

  DroneBattery() : batteryCapacity(1000.0f), batteryLevel(1000.0f), powerConsumption(0.0f) {};
};

// Container for each Drone's Data
struct DroneData {
  std::string name;
  DroneType type;
  uint32_t droneID;

  DroneBody body;
  DroneAeroDynamics aeroDynamics;
  DroneRotors rotors;

  DroneControlSystem controlSystem;
  DroneControlIntegrators controlIntegrators;

  DroneSensors sensors;
  DroneBattery battery;

  // Safety failure
  bool rotorFailFlag;
  int failRotorIndex;

  float groundEffectFactor;

  DroneData() : name("Unnamed"), type(DroneType::MULTIROTOR), droneID(-1), rotorFailFlag(false), failRotorIndex(-1), groundEffectFactor(1.0f) {};
};

struct DroneFleet {
  std::vector<DroneData> drones;
};

