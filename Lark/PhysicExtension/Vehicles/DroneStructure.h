#include <array>
#include "Utils/MathTypes.h"

namespace lark::drones {

    struct InertiaProperties {
        /// @brief Total mass of the drone in kilograms
        float mass;  // kg

        /// @brief Principal moments of inertia in kg*m^2
        /// @details Diagonal elements of the inertia tensor:
        ///   x: Ixx - resistance to rotation around X-axis (roll)
        ///   y: Iyy - resistance to rotation around Y-axis (pitch)
        ///   z: Izz - resistance to rotation around Z-axis (yaw)
        math::v3 principal_inertia;  // [Ixx, Iyy, Izz] in kg*m^2

        /// @brief Products of inertia in kg*m^2
        /// @details Off-diagonal elements of the inertia tensor:
        ///   x: Ixy - coupling between X and Y axis rotations
        ///   y: Iyz - coupling between Y and Z axis rotations
        ///   z: Ixz - coupling between X and Z axis rotations
        /// @note For symmetric drones, these are typically zero
        math::v3 product_inertia;   // [Ixy, Iyz, Ixz] in kg*m^2

        [[nodiscard]] math::m3x3 GetInertiaMatrix() const {
            return {
                principal_inertia.x, product_inertia.x, product_inertia.z,
                product_inertia.x, principal_inertia.y, product_inertia.y,
                product_inertia.z, product_inertia.y, principal_inertia.z
                };
        };

        [[nodiscard]] math::m3x3 GetInverseInertiaMatrix() const {
            return glm::inverse(GetInertiaMatrix());
        }

        [[nodiscard]] math::v3 GetWeight() const {
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
        std::array<math::v3, num_rotors> rotor_positions;  // meters

        /// @brief Rotor spin directions for torque balancing
        /// @details Motor rotation directions to balance yaw torque:
        ///   +1: Counter-clockwise (CCW) when viewed from above
        ///   -1: Clockwise (CW) when viewed from above
        /// Typical pattern: [CCW, CW, CCW, CW] for X-configuration
        std::array<int, num_rotors> rotor_directions;

        /// @brief IMU sensor position relative to center of mass
        /// @details Location of inertial measurement unit in body frame
        math::v3 imu_position;  // meters

        /// @brief Arm length from center to rotor (derived parameter)
        /// @note This is typically √2 * distance to rotor in X-config
        float arm_length() const {
            // Calculate from first rotor position (assuming symmetric)
            return std::sqrt(rotor_positions[0].x * rotor_positions[0].x +
                            rotor_positions[0].y * rotor_positions[0].y);
        }
    };

    struct AeroDynamicsProperties {
        /// parasitic drag in body x axis, N/(m/s)**2
        /// parasitic drag in body y axis, N/(m/s)**2
        /// parasitic drag in body z axis, N/(m/s)**2
        math::v3 parasitic_drag;

        [[nodiscard]] math::m3x3 GetDragMatrix() const {
            return {
                parasitic_drag.x, 0, 0,
                0, parasitic_drag.y, 0,
                0, 0, parasitic_drag.z
            };
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

        [[nodiscard]] math::m3x3 GetRotorDragMatrix() const {
            return {
                k_d, 0, 0,
                0, k_d, 0,
                0, 0, k_z
            };
        }
    };

    struct MotorProperties {
        // Motor response time in seconds
        float tau_m;
        // rad/s
        float rotor_speed_min;
        // rad/s
        float rotor_speed_maxt;
        // rad/s
        float motor_noise_std;

    };
    struct LowerLevelControllerProperties {
        // The body rate P gain (for cmd_ctbr)
        int k_w;
        // The *world* velocity P gain (for cmd_vel)
        int k_v;
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
        LowerLevelControllerProperties lower_level_controller_properties;
    };
}
