#include "ISA.h"
#include <stdexcept>

namespace lark::models {

namespace {
    // Helper functions to improve calculation precision
    float calculate_temperature(float altitude) {
        if (altitude <= ISA_TROPOPAUSE_ALTITUDE) {
            return ISA_SEA_LEVEL_TEMPERATURE + ISA_LAPSE_RATE * altitude;
        }
        return ISA_TROPOPAUSE_TEMPERATURE;
    }

    float calculate_pressure(float altitude, float temperature) {
        if (altitude <= ISA_TROPOPAUSE_ALTITUDE) {
            const float exponent = (-ISA_GRAVITY) / (ISA_GAS_CONSTANT * ISA_LAPSE_RATE);
            const float temp_ratio = temperature / ISA_SEA_LEVEL_TEMPERATURE;
            return ISA_SEA_LEVEL_PRESSURE * std::pow(temp_ratio, exponent);
        } else {
            const float tropo_temp_ratio = ISA_TROPOPAUSE_TEMPERATURE / ISA_SEA_LEVEL_TEMPERATURE;
            const float base_exponent = (-ISA_GRAVITY) / (ISA_GAS_CONSTANT * ISA_LAPSE_RATE);
            const float base_pressure = ISA_SEA_LEVEL_PRESSURE * std::pow(tropo_temp_ratio, base_exponent);

            const float above_tropo_exponent = (-ISA_GRAVITY * (altitude - ISA_TROPOPAUSE_ALTITUDE)) /
                                             (ISA_GAS_CONSTANT * ISA_TROPOPAUSE_TEMPERATURE);
            return base_pressure * std::exp(above_tropo_exponent);
        }
    }

    float calculate_density(float pressure, float temperature) {
        return pressure / (ISA_GAS_CONSTANT * temperature);
    }

    float calculate_viscosity(float temperature) {
        constexpr float SUTHERLAND_TEMP = 273.15f;
        constexpr float SUTHERLAND_C = 110.4f;
        constexpr float SUTHERLAND_REF_VISC = 1.716e-5f;

        const float temp_ratio = temperature / SUTHERLAND_TEMP;
        return SUTHERLAND_REF_VISC * std::pow(temp_ratio, 1.5f) *
               ((SUTHERLAND_TEMP + SUTHERLAND_C) / (temperature + SUTHERLAND_C));
    }

    float calculate_speed_of_sound(float temperature) {
        return std::sqrt(ISA_GAMMA * ISA_GAS_CONSTANT * temperature);
    }
}

AtmosphericConditions calculate_atmospheric_conditions(float altitude, float velocity) {
    if (altitude < 0.0f) {
        throw std::invalid_argument("Altitude cannot be negative");
    }
    if (altitude > 86000.0f) {
        throw std::out_of_range("Altitude exceeds valid range (0-86km)");
    }

    AtmosphericConditions conditions{};

    // Calculate atmospheric parameters with maximum precision
    conditions.temperature = calculate_temperature(altitude);
    conditions.pressure = calculate_pressure(altitude, conditions.temperature);
    conditions.density = calculate_density(conditions.pressure, conditions.temperature);
    conditions.viscosity = calculate_viscosity(conditions.temperature);
    conditions.speed_of_sound = calculate_speed_of_sound(conditions.temperature);

    // Calculate Mach number if velocity is non-zero
    conditions.mach_factor = (conditions.speed_of_sound > 0.0f) ?
        velocity / conditions.speed_of_sound : 0.0f;

    return conditions;
}

} // namespace lark::models