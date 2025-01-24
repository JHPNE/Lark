#pragma once
#include "ModelConstants.h"
#include "ISA.h"
#include <btBulletDynamicsCommon.h>

namespace lark::models {
  struct VortexState {
    btVector3 induced_velocity;    // Induced velocity from tip vortices
    float core_radius;             // Vortex core radius
    float circulation_strength;    // Vortex circulation strength
    float wake_age;               // Age of the vortex wake
    float dissipation_factor;     // Vortex dissipation factor
  };

  struct VortexParameters {
    float blade_tip_speed;        // Blade tip velocity
    float blade_chord;            // Blade chord length
    float effective_aoa;          // Effective angle of attack
    float blade_span;             // Blade span length
    int blade_count;              // Number of blades
  };

  VortexState calculate_tip_vortex(
      const VortexParameters& params,
      float air_density,
      float rotor_speed,
      float forward_velocity,
      const btVector3& rotor_position,
      const btVector3& evaluation_point,
      float delta_time);
}