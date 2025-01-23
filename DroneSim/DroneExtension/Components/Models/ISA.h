#pragma once
#include <algorithm>
#include <cmath>
#include "ModelConstants.h"

namespace lark::models {
    struct AtmosphericConditions {
        float density;      // kg/m³
        float temperature;  // K
        float pressure;     // Pa
        float viscosity;    // kg/(m·s)
        float mach_factor;  // dimensionless
        float speed_of_sound; // m/s
    };

    AtmosphericConditions calculate_atmospheric_conditions(float altitude, float velocity);
}