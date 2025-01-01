#include "DroneUpdate.h"
#include "SensorSimulation.h"
#include "ControlSystem.h"
#include "Aerodynamics.h"
#include "Safety.h"

void updateDroneSystem(DroneFleet& fleet, float deltaTime) {
  if (deltaTime <= 0.0f) return;

  for (auto& drone : fleet.drones) {
    // Validate drone state
    if (!drone.body.fuselageBody || !drone.body.fuselageBody->getMotionState()) {
      continue;
    }

    // 1. Simulate sensors with safety checks
    simulateSensors(drone);

    // 2. Update control system
    updateDroneControl(drone, deltaTime);

    // 3. Apply forces
    applyAeroAndPropForces(drone, deltaTime);

    // 4. Handle battery usage with bounds checking
    float powerUsed = 0.f;
    for (size_t i = 0; i < drone.rotors.rotorThrottle.size(); ++i) {
      powerUsed += drone.rotors.rotorThrottle[i] * drone.rotors.rotorMaxThrust * 0.01f;
    }

    drone.battery.powerConsumption = powerUsed;
    if (drone.battery.batteryLevel > 0.0f) {
      drone.battery.batteryLevel = std::max(0.0f,
          drone.battery.batteryLevel - powerUsed * deltaTime);
    }

    // 5. Safety checks
    checkAndApplyFailures(drone, deltaTime);
  }
}