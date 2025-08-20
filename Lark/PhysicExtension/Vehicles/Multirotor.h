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

        const DroneState& GetState() const { return m_state; }
        
    private:
        DroneDynamics m_dynamics;
        DroneState m_state;
        ControlAbstraction m_control_abstraction;
        bool m_aero;
        bool m_enable_ground;

        math::v4 GetCMDMotorSpeeds(DroneState state, ControlInput input);
    };
}