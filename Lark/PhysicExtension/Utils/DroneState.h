#pragma once
#include "Utils/MathTypes.h"

namespace lark::drones {

    struct DroneState {
        math::v3 position;
        math::v3 velocity;
        // Quat q
        math::v4 attitude;
        math::v3 body_rates;
        math::v3 wind;
        math::v4 rotor_speeds;
        math::v3 angular_velocity;
    };

    enum class ControlAbstraction {
        /// @brief Direct motor speed control (rad/s)
        CMD_MOTOR_SPEEDS,

        /// @brief Individual rotor thrust commands (N)
        CMD_MOTOR_THRUSTS,

        /// @brief Collective thrust (N) + body angular rates (rad/s)
        CMD_CTBR,

        /// @brief Collective thrust (N) + body moments (N⋅m)
        CMD_CTBM,

        /// @brief Collective thrust (N) + attitude quaternion
        CMD_CTATT,

        /// @brief Velocity vector in world frame (m/s)
        CMD_VEL,

        /// @brief Acceleration vector in world frame (m/s²)
        CMD_ACC
    };

    struct ControlInput {
        // Motor-level commands
        math::v4 cmd_motor_speeds;    // rad/s - for CMD_MOTOR_SPEEDS
        math::v4 cmd_motor_thrusts;   // N - for CMD_MOTOR_THRUSTS

        // Force and moment commands
        float cmd_thrust;              // N - collective thrust for CMD_CTBR, CMD_CTBM, CMD_CTATT
        math::v3 cmd_moment;           // N⋅m - for CMD_CTBM

        // Attitude commands
        math::v4 cmd_q;                // quaternion [x,y,z,w] - for CMD_CTATT
        math::v3 cmd_w;                // rad/s - body rates for CMD_CTBR

        // High-level commands
        math::v3 cmd_v;                // m/s - velocity in world frame for CMD_VEL
        math::v3 cmd_acc;              // m/s² - acceleration in world frame for CMD_ACC


        // Constructor for easy initialization
        ControlInput() :
            cmd_motor_speeds(0.0f),
            cmd_motor_thrusts(0.0f),
            cmd_thrust(0.0f),
            cmd_moment(0.0f),
            cmd_q(0.0f, 0.0f, 0.0f, 0.0f), // identity quaternion
            cmd_w(0.0f),
            cmd_v(0.0f),
            cmd_acc(0.0f){}
    };
}