#include "BladeFlapping.h"

namespace lark::models {
    namespace {
        // Basic non-dimensional parameters
        float calculate_advance_ratio(float forward_velocity, float rotor_speed, float radius) {
            if (rotor_speed < 1.0f) return 0.0f;
            return forward_velocity / (rotor_speed * RPM_TO_RAD * radius);
        }

        float calculate_thrust_coefficient(float mass, float air_density, float rotor_speed, float radius) {
            float disk_area = PI * radius * radius;
            float omega = rotor_speed * RPM_TO_RAD;
            float denominator = air_density * disk_area * std::pow(omega * radius, 2);
            if (denominator < 1e-6f) return 0.0f;
            return (mass * ISA_GRAVITY) / denominator;
        }

        // First-order flapping response
        float calculate_flapping_angle(const BladeProperties& props, float advance_ratio,
                                     float thrust_coeff, float rotor_speed) {
            // Basic first harmonic response
            float omega = rotor_speed * RPM_TO_RAD;
            if (omega < 1.0f) return 0.0f;

            // Calculate dimensionless natural frequency
            float omega_bar = std::sqrt(1.0f + props.spring_constant /
                                     (props.mass * std::pow(omega * props.blade_grip, 2)));

            // Phase lag angle
            float phi = std::atan2(props.lock_number/16.0f, omega_bar * omega_bar - 1.0f);

            // First harmonic flapping coefficient
            return advance_ratio * (thrust_coeff * std::cos(phi) /
                   std::sqrt(std::pow(omega_bar * omega_bar - 1.0f, 2) +
                           std::pow(props.lock_number/16.0f, 2)));
        }

        // Steady state coning
        float calculate_coning_angle(const BladeProperties& props, float thrust_coeff,
                                   float rotor_speed) {
            float omega = rotor_speed * RPM_TO_RAD;
            if (omega < 1.0f) return 0.0f;

            // Centrifugal stiffening
            float centrifugal = props.mass * std::pow(omega, 2) * props.hinge_offset;
            if (centrifugal < 1e-6f) return 0.0f;

            // Thrust moment about hinge
            float thrust_moment = thrust_coeff * props.lock_number / 6.0f;

            return std::atan2(thrust_moment, 1.0f + props.spring_constant/centrifugal);
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

        // Calculate non-dimensional parameters
        float advance_ratio = calculate_advance_ratio(forward_velocity, rotor_speed, props.blade_grip);
        float thrust_coeff = calculate_thrust_coefficient(props.mass, air_density, rotor_speed, props.blade_grip);

        // Calculate primary angles
        state.flapping_angle = calculate_flapping_angle(props, advance_ratio, thrust_coeff, rotor_speed);
        state.coning_angle = calculate_coning_angle(props, thrust_coeff, rotor_speed);

        // Calculate blade dynamics
        float omega = rotor_speed * RPM_TO_RAD;
        if (omega > 1.0f) {
            // Update flapping rate based on quasi-steady assumption
            state.flapping_rate = -advance_ratio * omega * std::sin(omega * delta_time);

            // Lead-lag motion from Coriolis
            state.lead_lag_angle = -2.0f * state.flapping_angle * state.flapping_rate / omega;
        }

        // Calculate TPP normal vector
        btQuaternion shaft_rotation(btVector3(1, 0, 0), shaft_tilt);
        btQuaternion flap_rotation(btVector3(0, 1, 0), state.flapping_angle);
        btMatrix3x3 rotation_matrix;
        rotation_matrix.setRotation(shaft_rotation * flap_rotation);
        state.tip_path_plane = rotation_matrix * btVector3(0, 1, 0);

        // Calculate disk loading
        float disk_area = PI * std::pow(props.blade_grip, 2);
        state.disk_loading = thrust_coeff * (0.5f * air_density * std::pow(omega * props.blade_grip, 2));

        return state;
    }
}