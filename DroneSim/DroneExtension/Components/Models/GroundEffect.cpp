#include "GroundEffect.h"

namespace lark::models {
    namespace {
        float calculate_base_ground_effect(float normalized_height, float thrust_coefficient) {
            if (normalized_height < 0.1f) return 1.4f; // Maximum theoretical increase
            if (normalized_height > 2.0f) return 1.0f;

            // Enhanced Cheeseman & Bennett model with empirical corrections
            float base_factor = 1.0f / (1.0f - std::pow(1.0f / (4.0f * normalized_height), 2));

            // Non-linear thrust coefficient influence
            float thrust_correction = 1.0f + 0.1f * std::pow(thrust_coefficient / 0.02f, 0.5f);

            // Enhanced proximity modeling
            float proximity_factor = 1.0f;
            if (normalized_height < 0.5f) {
                float ratio = normalized_height / 0.5f;
                proximity_factor = 0.9f + 0.1f * std::pow(ratio, 0.5f);
            }

            return std::min(base_factor * thrust_correction * proximity_factor, 1.4f);
        }

        float calculate_recirculation(float normalized_height,
                                    float velocity_magnitude,
                                    float collective_pitch) {
            if (normalized_height >= 1.0f) return 1.0f;

            // Enhanced recirculation model with velocity scaling
            float height_factor = std::pow(1.0f - (normalized_height / 1.0f), 0.5f);
            float velocity_factor = std::exp(-velocity_magnitude / 5.0f);

            // Improved pitch influence modeling
            float pitch_factor = 1.0f + 0.15f * std::abs(std::sin(collective_pitch));

            return 1.0f - 0.2f * height_factor * velocity_factor * pitch_factor;
        }

        float calculate_power_ratio(float thrust_multiplier, float normalized_height) {
            // Enhanced power calculation with viscous effects
            float base_power_ratio = 1.0f / std::pow(thrust_multiplier, 1.5f);

            // Improved near-ground power modeling
            if (normalized_height < 0.5f) {
                float viscous_factor = 0.15f * std::pow(1.0f - normalized_height / 0.5f, 0.7f);
                base_power_ratio += viscous_factor;
            }

            return base_power_ratio;
        }
    }

    GroundEffectState calculate_ground_effect(
        const GroundEffectParams& params,
        float height_agl,
        const AtmosphericConditions& conditions) {

        GroundEffectState state{};

        // Calculate normalized height (in rotor diameters)
        float normalized_height = height_agl / (2.0f * params.rotor_radius);
        state.effective_height = height_agl;

        // Calculate thrust multiplier
        state.thrust_multiplier = calculate_base_ground_effect(
            normalized_height,
            params.thrust_coefficient
        );

        // Calculate recirculation effects
        float velocity_magnitude = params.velocity.length();
        state.recirculation_factor = calculate_recirculation(
            normalized_height,
            velocity_magnitude,
            params.collective_pitch
        );

        // Apply recirculation to thrust multiplier
        state.thrust_multiplier *= state.recirculation_factor;

        // Calculate induced power ratio
        state.induced_power_ratio = calculate_power_ratio(
            state.thrust_multiplier,
            normalized_height
        );

        // Set terrain normal (assuming flat ground)
        state.surface_normal = btVector3(0, 1, 0);

        return state;
    }
}