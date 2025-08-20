#pragma once
#include "DroneStructure.h"
#include "DroneState.h"

namespace lark::drones {
    class Multirotor {
    public:
        explicit Multirotor(QuadParams quad_params, DroneState drone_state, ControlAbstraction control_abstraction) {
            m_quad_params = quad_params;
            m_state = drone_state;
            m_control = control_abstraction;
            aero = true;
            ground = false;
            m_weight = quad_params.inertia_properties.GetWeight();
            m_torqueThrustRatio = quad_params.rotor_properties.GetTorqueThrustRatio();

            //TODO: Geometry
        }


    private:
        QuadParams m_quad_params;
        ControlAbstraction m_control;
        DroneState m_state;

        math::v3 m_weight;
        float m_torqueThrustRatio;
        bool aero;
        bool ground;
    };
}
