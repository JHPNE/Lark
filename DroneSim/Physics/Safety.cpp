#include "Safety.h"

void checkAndApplyFailures(DroneData& drone, float deltaTime)
{
  static std::default_random_engine rng(
      (unsigned)std::chrono::system_clock::now().time_since_epoch().count()
  );
  static std::uniform_real_distribution<float> dist(0.0f, 1.0f);

  // Random chance that a rotor fails
  if (!drone.rotorFailFlag && dist(rng) < 0.0001f) // e.g., 1 in 10,000 each update
  {
    if (drone.rotors.rotorCount > 0)
    {
      drone.rotorFailFlag = true;

      float randVal = dist(rng); // [0,1)
      int chosenRotor = static_cast<int>(std::floor(randVal * drone.rotors.rotorCount));

      // Clamp to avoid out-of-range
      if (chosenRotor >= static_cast<int>(drone.rotors.rotorCount))
        chosenRotor = drone.rotors.rotorCount - 1;

      drone.failRotorIndex = chosenRotor;
    }
  }

  // Battery depletion
  if (drone.battery.batteryLevel <= 0.0f)
  {
    // All rotors effectively fail
    for (float &throttle : drone.rotors.rotorThrottle)
      throttle = 0.0f;
  }
}
