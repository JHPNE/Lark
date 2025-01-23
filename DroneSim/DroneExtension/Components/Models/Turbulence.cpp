#include "Turbulence.h"

namespace lark::models {
    namespace {
        constexpr float PI = glm::pi<float>();

        float calculate_von_karman_spectrum(float frequency, float airspeed, float length_scale, float turbulence_intensity = TURBULENCE_BASE_INTENSITY) {
            const float omega = 2.0f * PI * frequency / airspeed;
            const float sigma_u_squared = turbulence_intensity * turbulence_intensity;
            const float constant_factor = sigma_u_squared * (2.0f * length_scale) / PI;
            const float denominator = std::pow(1.0f + std::pow(1.339f * length_scale * omega, 2), 5.0f / 6.0f);

            return constant_factor / denominator;
        }
    }

    TurbulenceState calculate_turbulence(float altitude, float airspeed, const models::AtmosphericConditions& conditions, float delta_time) {
        TurbulenceState state{};

        // calculate base turbulence intensity based on alt
        float base_intensity = TURBULENCE_BASE_INTENSITY * std::exp(-altitude * TURBULENCE_HEIGHT_FACTOR);

        // adjust for temperature variations
        float temp_variation = std::abs(conditions.temperature - models::ISA_SEA_LEVEL_TEMPERATURE);
        float temp_contribution = TURBULENCE_TEMP_FACTOR * (temp_variation / models::ISA_SEA_LEVEL_TEMPERATURE);

        // turb intensity
        state.intensity = std::min(base_intensity + temp_contribution, MAX_TURBULENCE_INTENSITY);

        state.length_scale = std::max(altitude * 0.1f, MIN_TURBULENCE_SCALE);

        state.time_scale = state.length_scale / std::max(airspeed, 1.0f);

        for (int i = 0; i < 3; ++i) {
            float frequency = 1.0f / state.time_scale;
            float spectrum = calculate_von_karman_spectrum(frequency, airspeed, state.length_scale);
            float amplitude = std::sqrt(spectrum * state.intensity);

            float phase = delta_time * frequency;
            float turbulence = amplitude * std::sin(phase + i * 2.0f * PI / 3.0f);

            state.velocity[i] = turbulence * KOLMOGOROV_CONSTANT;
            state.angular_velocity[i] = turbulence * 0.5f/state.length_scale;
        }

        return state;
    }
}