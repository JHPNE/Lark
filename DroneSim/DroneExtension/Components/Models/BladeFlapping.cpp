#include "BladeFlapping.h"

#include <iostream>

namespace lark::models {
    namespace {
        // Calculate blade inertia about flapping hinge
        float calculate_blade_inertia(const BladeProperties& props) {
            return props.mass * std::pow(props.blade_grip, 2) / 3.0f;
        }

        // Calculate aerodynamic moment about flapping hinge
        float calculate_aero_moment(const BladeProperties& props, float air_density,
                      float rotor_speed, float collective_pitch,
                      float forward_velocity, float beta) {
            float omega = rotor_speed;
            float total_radius = props.hinge_offset + props.blade_grip;
            float chord = 0.1f * total_radius; // 10% of total radius

            const int num_elements = 10;
            float dr = props.blade_grip / num_elements;
            float total_moment = 0.0f;

            for(int i = 1; i <= num_elements; i++) {
                float r = i * dr; // Distance from hinge
                float Ut = omega * (props.hinge_offset + r);
                float Up = forward_velocity * std::sin(beta);

                float alpha = collective_pitch - std::atan2(Up, Ut);
                alpha = std::clamp(alpha, -0.3f, 0.3f);

                float Mach = std::sqrt(Ut*Ut + Up*Up) / 340.0f;
                float Cl = (2.0f * PI * alpha) / std::sqrt(1.0f - Mach*Mach);

                float q = 0.5f * air_density * (Ut*Ut + Up*Up);
                float dL = q * Cl * chord * dr;
                total_moment += dL * (props.hinge_offset + r);
            }

            return total_moment * props.lock_number / 8.0f; // Remove Lock number scaling
        }

        float calculate_centrifugal_stiffness(const BladeProperties& props, float rotor_speed) {
            float omega = rotor_speed;
            float radius = props.blade_grip;
            const int num_elements = 10;
            float dr = radius / num_elements;
            float dm = props.mass / num_elements;
            float sum_stiffness = 0.0f;

            for (int i = 1; i <= num_elements; ++i) {
                float r = i * dr;
                float distance_from_shaft = props.hinge_offset + r;
                sum_stiffness += dm * omega * omega * distance_from_shaft * distance_from_shaft;
            }
            return sum_stiffness;
        }

        // Calculate centrifugal moment
        float calculate_centrifugal_moment(const BladeProperties& props,
                                         float rotor_speed, float beta) {
            float radius = props.blade_grip;
            float omega = rotor_speed;

            // Integrate centrifugal force along blade span
            const int num_elements = 10;
            float dr = radius / num_elements;
            float dm = props.mass / num_elements;
            float total_moment = 0.0f;

            for(int i = 1; i <= num_elements; i++) {
                float r = i * dr;
                float Fc = dm * omega * omega * (props.hinge_offset + r);
                float moment_arm = (props.hinge_offset + r) * std::sin(beta);
                total_moment -= Fc * moment_arm; // Negative for restoring moment
            }

            // Add spring moment
            total_moment -= props.spring_constant * beta;

            return total_moment;
        }

        // Integrate blade motion using RK4
        void integrate_blade_motion(float& beta, float& beta_dot,
                                  float aero_moment, float centrifugal_moment,
                                  const BladeProperties& props, float dt) {
            float I_beta = calculate_blade_inertia(props);

            // RK4 integration for second-order ODE
            auto deriv = [&](float b, float bd) -> std::pair<float, float> {
                float beta_dot = bd;
                float beta_ddot = (aero_moment + centrifugal_moment -
                                 props.spring_constant * b) / I_beta;
                return {beta_dot, beta_ddot};
            };

            // RK4 coefficients
            float k1_beta, k1_omega, k2_beta, k2_omega, k3_beta, k3_omega, k4_beta, k4_omega;
            std::pair<float, float> k1 = deriv(beta, beta_dot);
            k1_beta = k1.first;
            k1_omega = k1.second;

            std::pair<float, float> k2 = deriv(beta + dt*k1_beta/2, beta_dot + dt*k1_omega/2);
            k2_beta = k2.first;
            k2_omega = k2.second;

            std::pair<float, float> k3 = deriv(beta + dt*k2_beta/2, beta_dot + dt*k2_omega/2);
            k3_beta = k3.first;
            k3_omega = k3.second;

            std::pair<float, float> k4 = deriv(beta + dt*k3_beta, beta_dot + dt*k3_omega);
            k4_beta = k4.first;
            k4_omega = k4.second;

            // Update states
            beta += (dt/6.0f) * (k1_beta + 2*k2_beta + 2*k3_beta + k4_beta);
            beta_dot += (dt/6.0f) * (k1_omega + 2*k2_omega + 2*k3_omega + k4_omega);
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
        float I_beta = calculate_blade_inertia(props);
        float total_radius = props.hinge_offset + props.blade_grip;
        float chord = 0.1f * total_radius;
        float disk_area = PI * std::pow(total_radius, 2);

        // Azimuth calculation for cyclic effects
        static float azimuth = 0.0f;
        azimuth += rotor_speed * delta_time;
        azimuth = std::fmod(azimuth, 2.0f * PI);

        // Aerodynamic force calculation with cyclic pitch
        float effective_pitch = collective_pitch + cyclic_pitch * std::sin(azimuth);
        const int num_elements = 10;
        float dr = props.blade_grip / num_elements;
        float aero_moment = 0.0f;

        for (int i = 1; i <= num_elements; i++) {
            float r = i * dr;
            float local_radius = props.hinge_offset + r;
            float Ut = rotor_speed * local_radius;
            float Up = forward_velocity * std::sin(state.flapping_angle);

            float alpha = effective_pitch - std::atan2(Up, Ut);
            alpha = std::clamp(alpha, -0.3f, 0.3f);

            float Mach = std::sqrt(Ut*Ut + Up*Up) / 340.0f;
            float Cl = (2.0f * PI * alpha) / std::sqrt(std::max(1.0f - Mach*Mach, 0.5f));
            float q = 0.5f * air_density * (Ut*Ut + Up*Up);
            float dL = q * Cl * chord * dr;
            aero_moment += dL * local_radius;
        }

        // Calculate centrifugal stiffness and moment
        float centrifugal_stiffness = calculate_centrifugal_stiffness(props, rotor_speed);
        float cf_moment = -centrifugal_stiffness * state.flapping_angle;

        // Spring moment
        float spring_moment = -props.spring_constant * state.flapping_angle;

        // Total moment
        float total_moment = aero_moment + cf_moment + spring_moment;

        // Update flapping motion using semi-implicit Euler
        float angular_accel = total_moment / I_beta;
        state.flapping_rate += angular_accel * delta_time;
        state.flapping_angle += state.flapping_rate * delta_time;

        // Calculate coning angle
        state.coning_angle = aero_moment / (centrifugal_stiffness + props.spring_constant);
        state.coning_angle = std::clamp(state.coning_angle, 0.0f, 0.2f);

        // Calculate lead-lag due to Coriolis
        if (std::abs(rotor_speed) > 1.0f) {
            state.lead_lag_angle = -2.0f * state.flapping_angle * state.flapping_rate / rotor_speed;
        } else {
            state.lead_lag_angle = 0.0f;
        }

        // Tip path plane orientation
        btQuaternion shaft_rot(btVector3(1, 0, 0), shaft_tilt);
        btQuaternion flap_rot(btVector3(0, 1, 0), state.flapping_angle);
        btQuaternion cone_rot(btVector3(1, 0, 0), state.coning_angle);

        btMatrix3x3 rotation;
        rotation.setRotation(shaft_rot * flap_rot * cone_rot);
        state.tip_path_plane = rotation * btVector3(0, 1, 0);
        state.tip_path_plane.normalize();

        if (forward_velocity == 0.0f) {
            state.tip_path_plane = btVector3(0.0f, 0.0f, 1.0f);
        }

        // Calculate disk loading
        state.disk_loading = aero_moment / (total_radius * disk_area);

        return state;
    }
}