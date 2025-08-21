#pragma once
#include "PhysicExtension/Utils/DroneState.h"
#include "PhysicExtension/Utils/DroneStructure.h"

namespace lark::drones {

    // Shared dynamics calculations
    class DroneDynamics {
    public:
        explicit DroneDynamics(const QuadParams& quad_params) 
            : m_quad_params(quad_params) {
            
            m_weight = m_quad_params.inertia_properties.GetWeight();
            m_torque_thrust_ratio = m_quad_params.rotor_properties.GetTorqueThrustRatio();
            m_inertia_matrix = m_quad_params.inertia_properties.GetInertiaMatrix();
            m_inverse_inertia = m_quad_params.inertia_properties.GetInverseInertiaMatrix();
            m_drag_matrix = m_quad_params.aero_dynamics_properties.GetDragMatrix();
            m_rotor_drag_matrix = m_quad_params.rotor_properties.GetRotorDragMatrix();
            
            generateControlAllocationMatrix();
            extractRotorGeometry();
        }

        // Getters for shared properties
        const math::v3& GetWeight() const { return m_weight; }
        float GetTorqueThrustRatio() const { return m_torque_thrust_ratio; }
        const math::m4x4& GetControlAllocationMatrix() const { return f_to_TM; }
        const math::m4x4& GetInverseControlAllocationMatrix() const { return TM_to_f; }
        const math::m3x3& GetInertiaMatrix() const { return m_inertia_matrix; }
        const math::m3x3& GetInverseInertia() const { return m_inverse_inertia; }
        const QuadParams& GetQuadParams() const { return m_quad_params; }
        const glm::mat4x3 GetRotorGeometry() const { return m_rotor_geometry;}

    private:
        void generateControlAllocationMatrix() {
            glm::mat4 matrix(0.0f);
            
            for (size_t i = 0; i < m_quad_params.geometric_properties.num_rotors; ++i) {
                // Column i represents rotor i's contribution
                matrix[i][0] = 1.0f;  // Thrust
                matrix[i][1] = m_quad_params.geometric_properties.rotor_positions[i].y;  // Roll moment
                matrix[i][2] = -m_quad_params.geometric_properties.rotor_positions[i].x; // Pitch moment
                matrix[i][3] = m_torque_thrust_ratio * m_quad_params.geometric_properties.rotor_directions[i]; // Yaw moment
            }
            
            f_to_TM = matrix;
            TM_to_f = glm::inverse(f_to_TM);
        }

        void extractRotorGeometry() {
            // If rotor_positions is std::array<math::v3, 4>
            // Convert to a 4x3 matrix for easier linear algebra operations

            glm::mat4x3 rotor_geometry;  // 4 rotors, 3 coordinates each

            for (size_t i = 0; i < m_quad_params.geometric_properties.num_rotors; ++i) {
                math::v3 r = m_quad_params.geometric_properties.rotor_positions[i];

                // Set row i to rotor position
                rotor_geometry[i][0] = r.x;
                rotor_geometry[i][1] = r.y;
                rotor_geometry[i][2] = r.z;
            }

            m_rotor_geometry = rotor_geometry;
        }

    private:
        QuadParams m_quad_params;
        math::v3 m_weight;
        float m_torque_thrust_ratio;
        math::m4x4 f_to_TM;
        math::m4x4 TM_to_f;
        math::m3x3 m_inertia_matrix;
        math::m3x3 m_inverse_inertia;
        math::m3x3 m_drag_matrix;
        math::m3x3 m_rotor_drag_matrix;
        glm::mat4x3 m_rotor_geometry;
    };
}