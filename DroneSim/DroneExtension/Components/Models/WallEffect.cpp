#include "WallEffect.h"

namespace lark::models {
    namespace {
        float calculate_pressure_coefficient(
            float distance_ratio,
            float thrust_coefficient,
            float forward_velocity,
            float rotor_radius) {
            
            float dynamic_pressure = 0.5f * forward_velocity * forward_velocity;
            if (dynamic_pressure < 0.001f) dynamic_pressure = 0.001f;

            float base_coefficient = thrust_coefficient / (PI * std::pow(distance_ratio, 2));
            return base_coefficient * std::exp(-2.0f * distance_ratio);
        }

        float calculate_interference_factor(
            float distance_ratio,
            const btVector3& wall_normal,
            const btVector3& rotor_velocity) {
            
            float velocity_alignment = wall_normal.dot(rotor_velocity.normalized());
            float base_factor = 1.0f / (1.0f + distance_ratio);
            
            return base_factor * (1.0f + std::abs(velocity_alignment));
        }

        btVector3 calculate_image_effect(
            const btVector3& rotor_position,
            const btVector3& wall_normal,
            float wall_distance,
            float interference_factor) {
            
            btVector3 image_position = rotor_position + 2.0f * wall_distance * wall_normal;
            btVector3 influence_vector = rotor_position - image_position;
            
            return influence_vector.normalized() * interference_factor;
        }
    }

    WallState calculate_wall_effect(
        const WallParameters& params,
        float air_density,
        float forward_velocity,
        const btVector3& rotor_position,
        const btVector3& rotor_velocity,
        float collective_pitch) {

        WallState state{};

        // Calculate effective distance ratio (distance/rotor radius)
        float distance_ratio = params.wall_distance / params.rotor_radius;
        state.effective_distance = params.wall_distance;

        // Calculate thrust coefficient
        float disk_area = PI * params.rotor_radius * params.rotor_radius;
        float dynamic_pressure = 0.5f * air_density * forward_velocity * forward_velocity;
        if (dynamic_pressure < 0.001f) dynamic_pressure = 0.001f;
        
        float thrust_coefficient = params.thrust / (dynamic_pressure * disk_area);

        // Calculate pressure coefficient near wall
        state.pressure_coefficient = calculate_pressure_coefficient(
            distance_ratio,
            thrust_coefficient,
            forward_velocity,
            params.rotor_radius
        );

        // Calculate interference factor
        state.interference_factor = calculate_interference_factor(
            distance_ratio,
            params.wall_normal,
            rotor_velocity
        );

        // Calculate image effect (mirror method)
        btVector3 image_influence = calculate_image_effect(
            rotor_position,
            params.wall_normal,
            params.wall_distance,
            state.interference_factor
        );

        // Calculate induced force
        state.induced_force = image_influence * params.thrust * state.pressure_coefficient;

        // Calculate induced moment
        // Cross product of force application point (approximated at rotor radius) with induced force
        btVector3 force_arm = params.wall_normal * params.rotor_radius;
        state.induced_moment = force_arm.cross(state.induced_force);

        // Scale effects based on collective pitch to account for rotor orientation
        float pitch_factor = std::abs(std::sin(collective_pitch));
        state.induced_force *= pitch_factor;
        state.induced_moment *= pitch_factor;

        // Add wall-normal component scaling based on proximity
        if (distance_ratio < 2.0f) {
            float normal_scale = 1.0f - (distance_ratio / 2.0f);
            btVector3 normal_force = params.wall_normal * params.thrust * normal_scale;
            state.induced_force += normal_force;
        }

        return state;
    }
}