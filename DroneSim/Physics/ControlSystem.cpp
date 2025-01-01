#include "ControlSystem.h"
#include <algorithm>
#include <cmath>

void updateDroneControl(DroneData& drone, float deltaTime)
{
  if (drone.type == DroneType::MULTIROTOR || drone.type == DroneType::HYBRID)
  {
    // Example: altitude hold at 10m
    float targetAlt = 10.0f;
    float altError  = targetAlt - drone.sensors.sensedAltitude;

    // PID integrator
    drone.controlIntegrators.altIntegral += altError * deltaTime;
    // No derivative for brevity, but you can add (altError - altErrorPrev)/dt

    float altP = drone.controlSystem.altitude.x * altError;
    float altI = drone.controlSystem.altitude.y * drone.controlIntegrators.altIntegral;
    float altD = 0.0f; // if we had derivative

    float altControl = altP + altI + altD;

    // Map altControl to throttle offset
    float baseThrottle = 0.5f; // nominal hover
    float throttleCmd  = baseThrottle + altControl * 0.01f;

    // Distribute throttle to all rotors
    for (int r = 0; r < drone.rotors.rotorCount; ++r)
    {
      float newThrottle = throttleCmd;
      // clamp
      newThrottle = std::clamp(newThrottle, 0.0f, 1.0f);

     drone.rotors.rotorThrottle[r] = newThrottle;
    }
  }
  else if (drone.type == DroneType::FIXED_WING)
  {
    // Example: maintain level flight by controlling pitch (not shown in detail).
    // You might adjust elevator surfaces or throttle if you have a prop.
    // ...
  }
}