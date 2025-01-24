#pragma once
#include <btBulletDynamicsCommon.h>

namespace lark::models {
    // Basic Constants
    constexpr float PI = glm::pi<float>();
    constexpr float RAD_TO_RPM = 60.0f / (2.0f * PI);
    constexpr float RPM_TO_RAD = (2.0f * PI) / 60.0f;

    // ISA Model Constants
    constexpr float ISA_SEA_LEVEL_PRESSURE = 101325.0f;    // Pa
    constexpr float ISA_SEA_LEVEL_TEMPERATURE = 288.15f;   // K (15°C)
    constexpr float ISA_SEA_LEVEL_DENSITY = 1.225f;        // kg/m³
    constexpr float ISA_LAPSE_RATE = -0.0065f;             // K/m (up to troposphere)
    constexpr float ISA_GAS_CONSTANT = 287.05f;            // J/(kg·K)
    constexpr float ISA_GRAVITY = 9.80665f;                // m/s²
    constexpr float ISA_TROPOPAUSE_ALTITUDE = 11000.0f;    // m
    constexpr float ISA_TROPOPAUSE_TEMPERATURE = 216.65f;  // K
    constexpr float ISA_GAMMA = 1.4f;                      // Ratio of specific heats for air

    // Turbelence Model parameters
    constexpr float TURBULENCE_BASE_INTENSITY = 0.1f;
    constexpr float TURBULENCE_HEIGHT_FACTOR = 0.15f;
    constexpr float TURBULENCE_TEMP_FACTOR = 0.05f;
    constexpr float MAX_TURBULENCE_INTENSITY = 1.0f;
    constexpr float MIN_TURBULENCE_SCALE = 0.1f;
    constexpr float KOLMOGOROV_CONSTANT = 1.5f;
}
