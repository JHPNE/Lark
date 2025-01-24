#pragma once
#include "ModelConstants.h"
#include "ISA.h"
#include <algorithm>
#include <cmath>
#include <btBulletDynamicsCommon.h>

namespace lark::models {
  struct BladeState {
    float flapping_angle;       // Beta - current flapping angle
    float flapping_rate;        // Beta dot - angular velocity
    float coning_angle;         // Alpha - steady state coning
    float lead_lag_angle;       // Xi - lead-lag angle
    btVector3 tip_path_plane;   // TPP normal vector
    float disk_loading;         // Current disk loading
  };

  struct BladeProperties {
    float mass;                 // Mass of single blade
    float hinge_offset;         // Distance from shaft to flap hinge
    float lock_number;          // Gamma - blade inertia number
    float spring_constant;      // K_beta - flapping hinge spring
    float natural_frequency;    // omega_beta - natural frequency
    float blade_grip;           // Distance from hinge to blade start
  };

  // Main function to calculate blade state
  BladeState calculate_blade_state(
      const BladeProperties& props,
      float rotor_speed,          // Omega
      float forward_velocity,
      float air_density,
      float collective_pitch,
      float cyclic_pitch,
      float shaft_tilt,
      float delta_time);
  }