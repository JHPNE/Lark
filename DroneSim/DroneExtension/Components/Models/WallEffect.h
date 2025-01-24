#pragma once
#include "ModelConstants.h"
#include "ISA.h"
#include <btBulletDynamicsCommon.h>

namespace lark::models {
  struct WallState {
    btVector3 induced_force;      // Force induced by wall effect
    btVector3 induced_moment;     // Moment induced by wall effect
    float pressure_coefficient;   // Wall pressure coefficient
    float effective_distance;     // Effective distance to wall
    float interference_factor;    // Wall interference factor
  };

  struct WallParameters {
    btVector3 wall_normal;        // Normal vector to wall
    float wall_distance;          // Distance to wall
    float rotor_radius;           // Rotor radius
    float disk_loading;           // Rotor disk loading
    float thrust;                 // Current rotor thrust
  };

  WallState calculate_wall_effect(
      const WallParameters& params,
      float air_density,
      float forward_velocity,
      const btVector3& rotor_position,
      const btVector3& rotor_velocity,
      float collective_pitch);
}