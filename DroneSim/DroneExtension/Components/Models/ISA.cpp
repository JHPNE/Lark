#include "ISA.h"

namespace lark::models {

    AtmosphericConditions calculate_atmospheric_conditions(float altitude, float velocity) {
            AtmosphericConditions conditions{};

            // Ensure non-negative altitude
            altitude = std::max(0.0f, altitude);

            // Temperature calculation based on ISA model
            if (altitude <= ISA_TROPOPAUSE_ALTITUDE) {
                conditions.temperature = ISA_SEA_LEVEL_TEMPERATURE + ISA_LAPSE_RATE * altitude;
            } else {
                conditions.temperature = ISA_TROPOPAUSE_TEMPERATURE;
            }

            // Pressure calculation
            if (altitude <= ISA_TROPOPAUSE_ALTITUDE) {
                float exponent = -ISA_GRAVITY / (ISA_GAS_CONSTANT * ISA_LAPSE_RATE);
                conditions.pressure = ISA_SEA_LEVEL_PRESSURE *
                    std::pow(conditions.temperature / ISA_SEA_LEVEL_TEMPERATURE, exponent);
            } else {
                float base_pressure = ISA_SEA_LEVEL_PRESSURE *
                    std::pow(ISA_TROPOPAUSE_TEMPERATURE / ISA_SEA_LEVEL_TEMPERATURE,
                            -ISA_GRAVITY / (ISA_GAS_CONSTANT * ISA_LAPSE_RATE));
                float exponent = -ISA_GRAVITY * (altitude - ISA_TROPOPAUSE_ALTITUDE) /
                                (ISA_GAS_CONSTANT * ISA_TROPOPAUSE_TEMPERATURE);
                conditions.pressure = base_pressure * std::exp(exponent);
            }

            // Density from ideal gas law
            conditions.density = conditions.pressure / (ISA_GAS_CONSTANT * conditions.temperature);

            // Dynamic viscosity using Sutherland's law
            constexpr float SUTHERLAND_TEMP = 273.15f;
            constexpr float SUTHERLAND_C = 120.0f;
            constexpr float SUTHERLAND_REF_VISC = 1.716e-5f;

            conditions.viscosity = SUTHERLAND_REF_VISC *
                std::pow(conditions.temperature / SUTHERLAND_TEMP, 1.5f) *
                ((SUTHERLAND_TEMP + SUTHERLAND_C) / (conditions.temperature + SUTHERLAND_C));

            // Speed of sound and Mach number
            conditions.speed_of_sound = std::sqrt(ISA_GAMMA * ISA_GAS_CONSTANT * conditions.temperature);
            conditions.mach_factor = velocity / conditions.speed_of_sound;

            return conditions;
        }
}