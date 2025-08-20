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

            generateControlAllocationMatrix();

            //TODO: Geometry
        }

        math::v3 GetWeight() const {return m_weight;}
        math::m4x4 GetControlAllocationMatrix() const { return f_to_TM; }
        math::m4x4 GetInverseControlAllocationMatrix() const { return TM_to_f; }

    private:

        /// @brief Generate control allocation matrices
        /// @param geometry Drone geometric properties
        /// @param rotor_props Rotor properties (for k_m coefficient)
        void generateControlAllocationMatrix() {
            glm::mat4 matrix(0.0f);

            for (size_t i = 0; i < m_quad_params.geometric_properties.num_rotors; ++i) {
                // Column i represents rotor i's contribution
                matrix[i][0] = 1.0f;  // Thrust
                matrix[i][1] = m_quad_params.geometric_properties.rotor_positions[i].y;  // Roll
                matrix[i][2] = -m_quad_params.geometric_properties.rotor_positions[i].x; // Pitch
                matrix[i][3] = m_torqueThrustRatio * m_quad_params.geometric_properties.rotor_directions[i]; // Yaw
            }

            f_to_TM = matrix;
            TM_to_f = glm::inverse(f_to_TM);
        }

        QuadParams m_quad_params;
        ControlAbstraction m_control;
        DroneState m_state;

        math::v3 m_weight;
        float m_torqueThrustRatio;

        math::m4x4 f_to_TM;
        math::m4x4 TM_to_f;

        bool aero;
        bool ground;
    };
}
