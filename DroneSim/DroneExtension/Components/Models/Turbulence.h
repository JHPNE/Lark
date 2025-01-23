#pragma once
#include <algorithm>
#include <cmath>
#include "ModelConstants.h"
#include "ISA.h"
#include "../Rotor.h"

namespace lark::models {
    struct TurbulenceState {
        btVector3 velocity;          // Turbulent velocity components
        btVector3 angular_velocity;  // Turbulent angular velocity
        float intensity;            // Overall turbulence intensity
        float length_scale;         // Characteristic length scale
        float time_scale;           // Characteristic time scale
    };

    TurbulenceState calculate_turbulence(float altitude, float airspeed, const models::AtmosphericConditions& conditions, float delta_time);

}