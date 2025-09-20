#pragma once
#include "PhysicExtension/Utils/DroneState.h"
#include "PhysicExtension/Utils/DroneStructure.h"
#include "PhysicExtension/Utils/PhysicsMath.h"

namespace lark::drone
{

// Shared dynamics calculations
class DroneDynamics
{
  public:
    explicit DroneDynamics(const QuadParams &quad_params) : m_quad_params(quad_params)
    {

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
    const Vector3f &GetWeight() const { return m_weight; }
    float GetTorqueThrustRatio() const { return m_torque_thrust_ratio; }
    const Matrix4f &GetControlAllocationMatrix() const { return f_to_TM; }
    const Matrix4f &GetInverseControlAllocationMatrix() const { return TM_to_f; }
    const Matrix3f &GetInertiaMatrix() const { return m_inertia_matrix; }
    const Matrix3f &GetInverseInertia() const { return m_inverse_inertia; }
    const QuadParams &GetQuadParams() const { return m_quad_params; }
    const Matrix4x3f GetRotorGeometry() const { return m_rotor_geometry; }

  private:
    void generateControlAllocationMatrix()
    {
        Matrix4f matrix = Matrix4f::Zero();

        for (size_t i = 0; i < m_quad_params.geometric_properties.num_rotors; ++i)
        {
            matrix(i, 0) = 1.0f;                                                      // Thrust
            matrix(i, 1) = m_quad_params.geometric_properties.rotor_positions[i].y(); // Roll moment
            matrix(i, 2) =
                -m_quad_params.geometric_properties.rotor_positions[i].x(); // Pitch moment
            matrix(i, 3) = m_torque_thrust_ratio *
                           m_quad_params.geometric_properties.rotor_directions[i]; // Yaw moment
        }

        f_to_TM = matrix;
        TM_to_f = f_to_TM.inverse();
    }

    void extractRotorGeometry()
    {
        Matrix4x3f rotor_geometry;

        for (size_t i = 0; i < m_quad_params.geometric_properties.num_rotors; ++i)
        {
            rotor_geometry.row(i) = m_quad_params.geometric_properties.rotor_positions[i];
        }

        m_rotor_geometry = rotor_geometry;
    }

    QuadParams m_quad_params;
    Vector3f m_weight;
    float m_torque_thrust_ratio;
    Matrix4f f_to_TM;
    Matrix4f TM_to_f;
    Matrix3f m_inertia_matrix;
    Matrix3f m_inverse_inertia;
    Matrix3f m_drag_matrix;
    Matrix3f m_rotor_drag_matrix;
    Matrix4x3f m_rotor_geometry;
};
} // namespace lark::drones