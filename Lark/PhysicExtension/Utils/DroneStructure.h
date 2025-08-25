#pragma once
#include <array>
#include "PhysicExtension/Utils/PhysicsMath.h"

namespace lark::drones {
    using namespace physics_math;

    struct InertiaProperties {
        /// @brief Total mass of the drone in kilograms
        float mass;  // kg

        /// @brief Principal moments of inertia in kg*m^2
        /// @details Diagonal elements of the inertia tensor:
        ///   x: Ixx - resistance to rotation around X-axis (roll)
        ///   y: Iyy - resistance to rotation around Y-axis (pitch)
        ///   z: Izz - resistance to rotation around Z-axis (yaw)
        Vector3f principal_inertia;  // [Ixx, Iyy, Izz] in kg*m^2

        /// @brief Products of inertia in kg*m^2
        /// @details Off-diagonal elements of the inertia tensor:
        ///   x: Ixy - coupling between X and Y axis rotations
        ///   y: Iyz - coupling between Y and Z axis rotations
        ///   z: Ixz - coupling between X and Z axis rotations
        /// @note For symmetric drones, these are typically zero
        Vector3f product_inertia;   // [Ixy, Iyz, Ixz] in kg*m^2

        [[nodiscard]] Matrix3f GetInertiaMatrix() {
            Matrix3f I;
            I << principal_inertia.x(), product_inertia.x(), product_inertia.z(),
                 product_inertia.x(), principal_inertia.y(), product_inertia.y(),
                 product_inertia.z(), product_inertia.y(), principal_inertia.z();
            return I;
        }

        [[nodiscard]] Matrix3f GetInverseInertiaMatrix() {
            return GetInertiaMatrix().inverse();
        }

        [[nodiscard]] Vector3f GetWeight() const {
            return {0, 0, -mass * 9.81f};
        }
    };

    struct GeometricProperties {
        /// @brief Number of rotors on the drone
        static constexpr size_t num_rotors = 4;

        /// @brief Radius of each rotor in meters
        float rotor_radius;  // meters

        /// @brief Position of each rotor relative to center of mass
        /// @details Positions are in body frame coordinates:
        ///   - X: forward direction
        ///   - Y: right direction
        ///   - Z: down direction
        /// Standard quadcopter X-configuration at 45° angles
        std::array<Vector3f, num_rotors> rotor_positions;  // meters

        /// @brief Rotor spin directions for torque balancing
        /// @details Motor rotation directions to balance yaw torque:
        ///   +1: Counter-clockwise (CCW) when viewed from above
        ///   -1: Clockwise (CW) when viewed from above
        /// Typical pattern: [CCW, CW, CCW, CW] for X-configuration
        Vector4f rotor_directions;

        /// @brief IMU sensor position relative to center of mass
        /// @details Location of inertial measurement unit in body frame
        Vector3f imu_position;  // meters

        /// @brief Arm length from center to rotor (derived parameter)
        /// @note This is typically √2 * distance to rotor in X-config
        float arm_length() const {
            // Calculate from first rotor position (assuming symmetric)
            return std::sqrt(rotor_positions[0].x() * rotor_positions[0].x() +
                            rotor_positions[0].y() * rotor_positions[0].y());
        }
    };

    struct AeroDynamicsProperties {
        /// parasitic drag in body x axis, N/(m/s)**2
        /// parasitic drag in body y axis, N/(m/s)**2
        /// parasitic drag in body z axis, N/(m/s)**2
        Vector3f parasitic_drag;

        [[nodiscard]] Matrix3f GetDragMatrix() const {
            return parasitic_drag.asDiagonal();
        }
    };

    struct RotorProperties {
        /// @brief Thrust coefficient relating rotor speed to thrust force
        /// @details Thrust = k_eta * omega^2
        /// @units N/(rad/s)^2
        float k_eta;

        /// @brief Yaw moment coefficient relating rotor speed to reaction torque
        /// @details Moment = k_m * omega^2 (around rotor axis)
        /// @units Nm/(rad/s)^2
        float k_m;

        /// @brief Rotor drag coefficient for aerodynamic drag
        /// @details Drag force opposing rotor motion through air
        /// @units N/(rad*m/s^2) = kg/rad
        float k_d;

        /// @brief Induced inflow coefficient for momentum theory effects
        /// @details Models induced velocity effects on rotor performance
        /// @units N/(rad*m/s^2) = kg/rad
        float k_z;

        /// @brief Translational lift coefficient for forward flight
        /// @details Additional lift due to forward/sideways motion
        /// @units N/(m/s)^2 = kg/m
        float k_h;

        /// @brief Flapping moment coefficient for rotor blade flapping
        /// @details Moment due to cyclic blade flapping (typically zero for small drones)
        /// @units Nm/(rad*m/s^2) = kg*m/rad
        float k_flap;

        [[nodiscard]] float GetTorqueThrustRatio() const {
            return k_m/k_eta;
        }

        [[nodiscard]] Matrix3f GetRotorDragMatrix() const {
            return Vector3f(k_d, k_d, k_z).asDiagonal();
        }
    };

    struct MotorProperties {
        // Motor response time in seconds
        float tau_m;
        // rad/s
        float rotor_speed_min;
        // rad/s
        float rotor_speed_max;
        // rad/s
        float motor_noise_std;
    };

    struct ControlGains {
        Vector3f kp_pos = {6.5f, 6.5f, 15.0f};
        Vector3f kd_pos = {4.0f, 4.0f, 9.0f};
        float kp_att = 544.0f;
        float kd_att = 46.64f;
        Vector3f kp_vel = {0.65f, 0.65f, 1.5f};  // 0.1 * kp_pos
    };

    struct LowerLevelControllerProperties {
        // The body rate P gain (for cmd_ctbr)
        float k_w;
        // The *world* velocity P gain (for cmd_vel)
        float k_v;
        // The attitude P gain (for cmd_vel, cmd_acc, and cmd_ctatt)
        int kp_att;
        // The attitude D gain (for cmd_vel, cmd_acc, and cmd_ctatt)
        float kd_att;

    };

     /**
     * Contains all the properties fro quad drone/uav
     *
    */
    struct QuadParams {
        InertiaProperties inertia_properties;
        GeometricProperties geometric_properties;
        AeroDynamicsProperties aero_dynamics_properties;
        RotorProperties rotor_properties;
        MotorProperties motor_properties;
        ControlGains control_gains;
        LowerLevelControllerProperties lower_level_controller_properties;
    };

    struct TrajectoryPoint {
        // Position trajectory
        Vector3f position;           // meters
        Vector3f velocity;           // m/s  (x_dot in Python)
        Vector3f acceleration;       // m/s² (x_ddot in Python)
        Vector3f jerk;              // m/s³ (x_dddot - optional, not used in basic SE3)
        Vector3f snap;              // m/s⁴ (x_ddddot - optional, not used in basic SE3)

        // Yaw trajectory
        float yaw;                   // radians
        float yaw_dot;              // rad/s (yaw rate)
        float yaw_ddot;
    };
}
