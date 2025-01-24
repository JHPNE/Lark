#include "DynamicInflow.h"

namespace lark::models {
    namespace {
        btMatrix3x3 calculate_mass_matrix(
            float advance_ratio,
            float wake_angle) {

            // Pitt-Peters dynamic inflow mass matrix
            float vm = std::sqrt(1.0f + advance_ratio * advance_ratio);
            float chi = wake_angle;

            btMatrix3x3 mass;
            mass.setValue(
                8.0f/3.0f/PI,           0.0f,                   0.0f,
                0.0f,                   16.0f/45.0f/PI/vm,     0.0f,
                0.0f,                   0.0f,                   16.0f/45.0f/PI/vm
            );

            // Add wake skew effects
            if (std::abs(chi) > 0.001f) {
                mass[1][1] *= (1.0f - std::sin(chi));
                mass[2][2] *= (1.0f + std::sin(chi));
            }

            return mass;
        }

        btVector3 calculate_forcing_terms(
            float thrust_coefficient,
            float advance_ratio,
            float collective_pitch) {

            // Basic forcing terms from thrust and pitch
            float thrust_term = thrust_coefficient / 2.0f;
            float pitch_term = collective_pitch * advance_ratio;

            return btVector3(
                thrust_term,
                pitch_term * std::cos(advance_ratio),
                pitch_term * std::sin(advance_ratio)
            );
        }

        float calculate_wake_skew(
            float forward_velocity,
            float mean_inflow,
            float rotor_speed,
            float rotor_radius) {

            // Calculate wake skew angle using momentum theory
            float induced_velocity = mean_inflow * rotor_speed * rotor_radius;
            return std::atan2(forward_velocity, induced_velocity);
        }
    }
    InflowState calculate_inflow(
        float thrust_coefficient,
        float disk_loading,
        float forward_velocity,
        float rotor_radius,
        float air_density,
        const btVector3& rotor_normal,
        float collective_pitch,
        float delta_time) {

        InflowState state{};
        const float rotor_area = PI * rotor_radius * rotor_radius;
        const float hover_velocity = std::sqrt(disk_loading / (2.0f * air_density));
        const float rotor_speed = std::sqrt(thrust_coefficient * 2.0f * PI * rotor_radius);
        
        // Calculate advance ratio
        float advance_ratio = forward_velocity / (rotor_speed * rotor_radius);

        // Initial wake skew calculation
        state.wake_skew = calculate_wake_skew(forward_velocity, hover_velocity, 
                                            rotor_speed, rotor_radius);

        // Setup mass-flow parameter matrix
        btMatrix3x3 mass_matrix = calculate_mass_matrix(advance_ratio, state.wake_skew);
        btVector3 forcing_terms = calculate_forcing_terms(thrust_coefficient, 
                                                        advance_ratio, collective_pitch);

        // Solve for inflow components using time-accurate integration
        btVector3 current_inflow(state.mean_inflow, 
                               state.longitudinal_inflow,
                               state.lateral_inflow);

        // Pitt-Peters first order dynamic inflow equations
        btVector3 inflow_derivatives = mass_matrix.inverse() * 
            (forcing_terms - current_inflow);

        // Euler integration (could be enhanced to RK4 if needed)
        current_inflow += inflow_derivatives * delta_time;

        // Update state variables
        state.mean_inflow = current_inflow.x();
        state.longitudinal_inflow = current_inflow.y();
        state.lateral_inflow = current_inflow.z();

        // Calculate total induced velocity vector
        float total_inflow = state.mean_inflow + 
                           state.longitudinal_inflow * std::cos(state.wake_skew) +
                           state.lateral_inflow * std::sin(state.wake_skew);

        state.induced_velocity = rotor_normal * (total_inflow * rotor_speed * rotor_radius);

        // Calculate dynamic TPP angle
        state.dynamic_tpl = std::atan2(state.longitudinal_inflow, state.mean_inflow);

        return state;
    }


}