#include "BladeFlapping.h"

namespace lark::models {
    namespace {
        float calculate_coning_angle(
            const BladeProperties& props,
            float rotor_speed,
            float air_density,
            float collective_pitch) {

            // Simplified coning angle calculation based on thrust equilibrium
            float thrust_coefficient = props.lock_number * collective_pitch / 6.0f;
            float centrifugal_force = props.mass * rotor_speed * rotor_speed * props.hinge_offset;

            return std::atan2(thrust_coefficient, centrifugal_force);
        }

        float calculate_flapping_moment(
            const BladeProperties& props,
            float current_azimuth,
            float air_density,
            float forward_velocity,
            float collective_pitch,
            float cyclic_pitch) {

            // Calculate local velocity and angle of attack
            float advance_ratio = forward_velocity / (props.natural_frequency * props.blade_grip);
            float local_velocity = props.natural_frequency * props.blade_grip *
                                 std::sqrt(1.0f + 2.0f * advance_ratio * std::cos(current_azimuth) +
                                         advance_ratio * advance_ratio);

            // Calculate effective angle of attack including cyclic input
            float local_aoa = collective_pitch + cyclic_pitch * std::cos(current_azimuth);

            // Calculate aerodynamic moment
            float dynamic_pressure = 0.5f * air_density * local_velocity * local_velocity;
            float lift_coefficient = 2.0f * PI * local_aoa;  // Simple linear lift curve

            return dynamic_pressure * lift_coefficient * props.blade_grip * props.blade_grip;
        }
    }

    BladeState calculate_blade_state(
        const BladeProperties& props,
        float rotor_speed,
        float forward_velocity,
        float air_density,
        float collective_pitch,
        float cyclic_pitch,
        float shaft_tilt,
        float delta_time) {

        BladeState state{};

        // Calculate steady state coning angle
        state.coning_angle = calculate_coning_angle(props, rotor_speed, air_density, collective_pitch);

        // Numerical integration of flapping equation using RK4
        const float omega_squared = props.natural_frequency * props.natural_frequency;
        const float damping = props.lock_number * rotor_speed / 8.0f;

        auto flapping_derivative = [&](float beta, float beta_dot, float azimuth) {
            float moment = calculate_flapping_moment(props, azimuth, air_density,
                                                   forward_velocity, collective_pitch, cyclic_pitch);

            return -omega_squared * (beta - state.coning_angle)
                   - damping * beta_dot
                   + moment / (props.mass * props.hinge_offset * props.hinge_offset);
        };

        // RK4 integration
        const float dt = delta_time;
        const float k1 = flapping_derivative(state.flapping_angle, state.flapping_rate, 0.0f) * dt;
        const float k2 = flapping_derivative(state.flapping_angle + 0.5f * k1,
                                           state.flapping_rate, 0.5f * rotor_speed * dt) * dt;
        const float k3 = flapping_derivative(state.flapping_angle + 0.5f * k2,
                                           state.flapping_rate, 0.5f * rotor_speed * dt) * dt;
        const float k4 = flapping_derivative(state.flapping_angle + k3,
                                           state.flapping_rate, rotor_speed * dt) * dt;

        state.flapping_rate += (k1 + 2.0f * k2 + 2.0f * k3 + k4) / 6.0f;
        state.flapping_angle += state.flapping_rate * dt;

        // Calculate tip path plane orientation
        btVector3 normal(0, 1, 0);
        btTransform shaft_transform;
        shaft_transform.setIdentity();
        shaft_transform.setRotation(btQuaternion(btVector3(1, 0, 0), shaft_tilt));

        btTransform flap_transform;
        flap_transform.setIdentity();
        flap_transform.setRotation(btQuaternion(btVector3(0, 1, 0), state.flapping_angle));

        btTransform total_transform = shaft_transform * flap_transform;
        state.tip_path_plane = total_transform.getBasis() * normal;

        // Calculate disk loading
        float disk_area = PI * props.blade_grip * props.blade_grip;
        state.disk_loading = props.mass * ISA_GRAVITY / disk_area;

        return state;
    }
}