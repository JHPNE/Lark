// Multirotor.h
#pragma once
#include "PhysicExtension/Utils/DroneDynamics.h"

namespace lark::drones {
    class Multirotor {
    public:
        explicit Multirotor(const QuadParams& quad_params, 
                           const DroneState& initial_state,
                           ControlAbstraction control_abstraction,
                           bool aero = true,
                           bool enable_ground = false)
            : m_dynamics(quad_params),
              m_state(initial_state),
              m_control_abstraction(control_abstraction),
              m_aero(aero),
              m_enable_ground(enable_ground) {
        }

        void step(DroneState state, ControlInput input, float dt);
        void s_dot_fn(DroneState state, math::v4 cmd_rotor_speeds);
        void ComputeBodyWrench(math::v3 body_rate, math::v4 rotor_speeds, math::v3 body_airspeed_vector);

        const DroneState& GetState() const { return m_state; }
        
    private:
        DroneDynamics m_dynamics;
        DroneState m_state;
        ControlAbstraction m_control_abstraction;
        bool m_aero;
        bool m_enable_ground;

        math::v4 GetCMDMotorSpeeds(DroneState state, ControlInput input);
        // Method 1: Manual column-wise addition
        glm::mat3x4 computeLocalAirspeeds(const math::v3& body_airspeed_vector,
                                          const math::v3& body_rates,
                                          const glm::mat4x3& rotor_geometry) {

            math::m3x3 omega_hat = math::hatMap(body_rates);
            glm::mat3x4 rotor_geometry_T = glm::transpose(rotor_geometry);
            glm::mat3x4 rotation_velocities = omega_hat * rotor_geometry_T;

            glm::mat3x4 local_airspeeds;
            for (int i = 0; i < 4; ++i) {
                // Add body_airspeed_vector to each column
                local_airspeeds[i] = body_airspeed_vector + rotation_velocities[i];
            }

            return local_airspeeds;
        }
    };
}