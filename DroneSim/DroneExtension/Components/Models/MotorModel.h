#pragma once
#include "ModelConstants.h"
#include <algorithm>

namespace lark::models {
  struct MotorState {
    float current_torque;         // Current motor torque
    float power_consumption;      // Electrical power consumption
    float winding_temperature;    // Motor temperature
    float efficiency;            // Current motor efficiency
    float back_emf;             // Back EMF voltage
  };

  struct MotorParameters {
    float kv_rating;            // Motor KV rating (RPM/V)
    float resistance;           // Winding resistance (ohms)
    float inductance;           // Winding inductance (H)
    float inertia;             // Rotor inertia (kg*m^2)
    float thermal_resistance;   // Thermal resistance to ambient (K/W)
    float thermal_capacity;     // Thermal capacity (J/K)
    float voltage;             // Supply voltage
    float max_current;         // Maximum current rating
  };

  MotorState calculate_motor_state(
      const MotorParameters& params,
      float demanded_rpm,
      float load_torque,
      float ambient_temperature,
      float delta_time);
}