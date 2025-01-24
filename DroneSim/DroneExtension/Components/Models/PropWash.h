#pragma once
#include "ModelConstants.h"
#include "ISA.h"

namespace lark::models {
    struct PropWashField {
        btVector3 velocity;
        btVector3 vorticity;
        float intensity;
    };

    PropWashField calculate_prop_wash(btVector3 rotorNormal, const float rpm, const float area, float radius, int blade_count, const AtmosphericConditions& conditions, float thrust);
    float calculate_prop_wash_influence(const models::PropWashField& wash, const btVector3& wash_origin, const btVector3& affected_point, float rotor_radius);
}
