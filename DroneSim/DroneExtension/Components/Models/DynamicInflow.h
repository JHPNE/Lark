#pragma once
#include "ModelConstants.h"
#include "ISA.h"
#include <btBulletDynamicsCommon.h>

namespace lark::models {
  struct InflowState {
    float mean_inflow;         // λ0 - mean inflow ratio
    float longitudinal_inflow; // λ1s - fore-to-aft variation
    float lateral_inflow;      // λ1c - side-to-side variation
    btVector3 induced_velocity;// Total induced velocity vector
    float wake_skew;          // χ - wake skew angle
    float dynamic_tpl;        // Dynamic TPP angle
  };


  InflowState calculate_inflow(
      float thrust_coefficient,
      float disk_loading,
      float forward_velocity,
      float rotor_radius,
      float air_density,
      const btVector3& rotor_normal,
      float collective_pitch,
      float delta_time);
}