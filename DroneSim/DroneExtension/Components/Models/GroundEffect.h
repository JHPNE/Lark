#pragma once
// GroundEffect.h
#include "ModelConstants.h"
#include "ISA.h"
#include <btBulletDynamicsCommon.h>

namespace lark::models {
  struct GroundEffectState {
    float thrust_multiplier;      // Overall thrust multiplication factor
    float induced_power_ratio;    // Ratio of induced power with ground effect
    float recirculation_factor;   // Flow recirculation influence
    float effective_height;       // Effective height considering terrain
    btVector3 surface_normal;     // Normal vector of the ground surface
  };

  struct GroundEffectParams {
    float rotor_radius;           // Rotor radius in meters
    float disk_loading;           // Current disk loading (N/m²)
    float thrust_coefficient;     // Non-dimensional thrust coefficient
    float collective_pitch;       // Blade collective pitch angle
    btVector3 position;          // Current rotor position
    btVector3 velocity;          // Current velocity vector
  };

  GroundEffectState calculate_ground_effect(
      const GroundEffectParams& params,
      float height_agl,              // Height above ground level
      const AtmosphericConditions& conditions);
}