// RotorPhysics.h
#pragma once
#include "../../DroneData.h"
#include "../Models/ISA.h"
#include "../Models/BladeFlapping.h"
#include "../Models/DynamicInflow.h"
#include "../Models/PropWash.h"
#include "../Models/TipVortex.h"
#include "../Models/Turbulence.h"
#include "../Models/WallEffect.h"
#include "../Models/MotorModel.h"

namespace lark::rotor::physics {
  // Core physics calculations
  float calculate_thrust(const drone_data::RotorBody* data, const models::AtmosphericConditions& conditions);
  float calculate_power(const drone_data::RotorBody* data, float thrust, const models::AtmosphericConditions& conditions);

  // Model updates
  void update_blade_state(drone_data::RotorBody* data, float velocity,
                        const models::AtmosphericConditions& conditions, float deltaTime);
  void update_vortex_state(drone_data::RotorBody* data, float velocity,
                          const models::AtmosphericConditions& conditions, float deltaTime);
  void update_motor_state(drone_data::RotorBody* data,
                        const models::AtmosphericConditions& conditions, float deltaTime);

  // Environmental effects
  void apply_wall_effects(drone_data::RotorBody* data, float velocity,
                        const models::AtmosphericConditions& conditions);
  void apply_turbulence(drone_data::RotorBody* data,
                       const models::AtmosphericConditions& conditions, float deltaTime);
  void apply_prop_wash(drone_data::RotorBody* data,
                      const models::AtmosphericConditions& conditions,
                      const std::vector<drone_data::RotorBody*>& other_rotors);

  // Helper functions
  void initialize_blade_properties(drone_data::RotorBody* data);
  void initialize_motor_parameters(drone_data::RotorBody* data);
}