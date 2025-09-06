// Multirotor.h
#pragma once
#include "PhysicExtension/Utils/DroneDynamics.h"

namespace lark::drones {
    struct StateDot {
        Eigen::Vector3f vdot;
        Eigen::Vector3f wdot;
    };

    struct SDot {
        Eigen::Vector3f xdot;
        Eigen::Vector3f vdot;
        Eigen::Vector4f qdot;
        Eigen::Vector3f wdot;
        Eigen::Vector3f wind_dot;
        Eigen::Vector4f rotor_accel;
    };

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

        DroneState step(DroneState state, ControlInput input, float dt);
        StateDot stateDot(DroneState state, ControlInput input, float dt);
        SDot s_dot_fn(DroneState state, Vector4f cmd_rotor_speeds);
        std::pair<Vector3f, Vector3f> ComputeBodyWrench(const Vector3f& body_rate, Vector4f rotor_speeds, const Vector3f& body_airspeed_vector);

        const DroneState& GetState() const { return m_state; }
        
    private:
        DroneDynamics m_dynamics;
        DroneState m_state;
        ControlAbstraction m_control_abstraction;
        bool m_aero;
        bool m_enable_ground;

        Vector4f GetCMDMotorSpeeds(DroneState state, ControlInput input);

        Eigen::VectorXf PackState(const DroneState& state) {
            int num_rotors = m_dynamics.GetQuadParams().geometric_properties.num_rotors;
            Eigen::VectorXf s = Eigen::VectorXf::Zero(16 + num_rotors);

            s.segment<3>(0) = state.position;
            s.segment<3>(3) = state.velocity;
            s.segment<4>(6) = state.attitude; // Quaternion as [x,y,z,w]
            s.segment<3>(10) = state.body_rates;
            s.segment<3>(13) = state.wind;
            s.segment(16, num_rotors) = state.rotor_speeds;

            return s;
        }

        DroneState UnpackState(const Eigen::VectorXf& s) {
            DroneState state;

            state.position = s.segment<3>(0);
            state.velocity = s.segment<3>(3);
            state.attitude = s.segment<4>(6); // w,x,y,z constructor
            state.body_rates = s.segment<3>(10);
            state.wind = s.segment<3>(13);

            int num_rotors = m_dynamics.GetQuadParams().geometric_properties.num_rotors;
            state.rotor_speeds = s.segment(16, num_rotors);

            return state;
        }

        Vector3f GetCMDMoment(DroneState state, Vector3f att_err) {
            // Split the complex moment calculation into sub-terms
            Vector3f attitude_term = -m_dynamics.GetQuadParams().control_gains.kp_att * att_err;
            Vector3f rate_term = -m_dynamics.GetQuadParams().control_gains.kd_att * state.body_rates;
            Vector3f control_input = attitude_term + rate_term;
            Vector3f inertia_control = m_dynamics.GetInertiaMatrix() * control_input;

            // Compute the gyroscopic term separately
            Vector3f inertia_omega = m_dynamics.GetInertiaMatrix() * state.body_rates;
            Vector3f gyroscopic_term = state.body_rates.cross(inertia_omega);

            // Final moment
            return inertia_control + gyroscopic_term;
        }
    };
}