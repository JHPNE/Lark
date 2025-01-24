#include "TipVortex.h"

namespace lark::models {
    namespace {
        float calculate_circulation(
            float blade_chord,
            float blade_tip_speed,
            float effective_aoa,
            float air_density) {
            
            const float lift_slope = 2.0f * PI;
            float lift_coefficient = lift_slope * effective_aoa;
            return 0.5f * lift_coefficient * blade_chord * blade_tip_speed;
        }

        float calculate_core_radius(
            float blade_chord,
            float wake_age,
            float reynolds_number) {
            
            const float initial_core = 0.05f * blade_chord;
            const float growth_rate = 0.0001f;
            return initial_core * (1.0f + growth_rate * wake_age * std::sqrt(reynolds_number));
        }

        btVector3 calculate_induced_velocity(
            const btVector3& vortex_position,
            const btVector3& evaluation_point,
            float circulation,
            float core_radius) {
            
            btVector3 r = evaluation_point - vortex_position;
            float distance = r.length();
            
            if (distance < 0.001f) return btVector3(0, 0, 0);

            float velocity_magnitude = circulation / (2.0f * PI * distance);
            velocity_magnitude *= (1.0f - std::exp(-std::pow(distance/core_radius, 2)));
            
            btVector3 direction = r.cross(btVector3(0, 1, 0)).normalized();
            return direction * velocity_magnitude;
        }
    }

    VortexState calculate_tip_vortex(
        const VortexParameters& params,
        float air_density,
        float rotor_speed,
        float forward_velocity,
        const btVector3& rotor_position,
        const btVector3& evaluation_point,
        float delta_time) {

        VortexState state{};

        // Calculate Reynolds number for viscous effects
        float reynolds_number = (params.blade_tip_speed * params.blade_chord * air_density) / 
                              1.81e-5f; // Using standard air viscosity

        // Calculate circulation strength
        state.circulation_strength = calculate_circulation(
            params.blade_chord,
            params.blade_tip_speed,
            params.effective_aoa,
            air_density
        );

        // Update wake age
        state.wake_age += delta_time;
        state.dissipation_factor = std::exp(-state.wake_age / 5.0f); // 5-second characteristic time

        // Calculate core radius with viscous spreading
        state.core_radius = calculate_core_radius(
            params.blade_chord,
            state.wake_age,
            reynolds_number
        );

        // Initialize induced velocity
        state.induced_velocity.setZero();

        // Calculate contribution from each blade's tip vortex
        for (int i = 0; i < params.blade_count; ++i) {
            float azimuth = (2.0f * PI * i) / params.blade_count + rotor_speed * state.wake_age;
            
            // Helix parameters for tip vortex path
            float radial_position = params.blade_span;
            float vertical_displacement = -forward_velocity * state.wake_age;
            
            // Calculate vortex position
            btVector3 vortex_position = rotor_position + btVector3(
                radial_position * std::cos(azimuth),
                vertical_displacement,
                radial_position * std::sin(azimuth)
            );

            // Add induced velocity contribution from this vortex segment
            state.induced_velocity += calculate_induced_velocity(
                vortex_position,
                evaluation_point,
                state.circulation_strength * state.dissipation_factor,
                state.core_radius
            );
        }

        return state;
    }
}