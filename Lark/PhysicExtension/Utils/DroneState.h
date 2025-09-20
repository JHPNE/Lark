#pragma once
#include "PhysicExtension/Utils/PhysicsMath.h"

namespace lark::drone
{
using namespace lark::physics_math;

struct DroneState
{
    Vector3f position;
    Vector3f velocity;
    Vector4f attitude;   // Quaternion [x,y,z,w]
    Vector3f body_rates; // w
    Vector3f wind;
    Vector4f rotor_speeds;
};

enum class ControlAbstraction
{
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

struct ControlInput
{
    // Motor level commands
    Vector4f cmd_motor_speeds;  // rad/s - for CMD_MOTOR_SPEEDS
    Vector4f cmd_motor_thrusts; // N - for CMD_MOTOR_THRUSTS

    // Force and moment commands
    float cmd_thrust;    // N - collective thrust for CMD_CTBR, CMD_CTBM, CMD_CTATT
    Vector3f cmd_moment; // N⋅m - for CMD_CTBM

    // Attitude commands
    Vector4f cmd_q; // quaternion [x,y,z,w] - for CMD_CTATT
    Vector3f cmd_w; // rad/s - body rates for CMD_CTBR

    // High-level commands
    Vector3f cmd_v;   // m/s - velocity in world frame for CMD_VEL
    Vector3f cmd_acc; // m/s² - acceleration in world frame for CMD_ACC

    ControlInput()
        : cmd_motor_speeds(Vector4f::Zero()), cmd_motor_thrusts(Vector4f::Zero()), cmd_thrust(0.0f),
          cmd_moment(Vector3f::Zero()), cmd_q(0.0f, 0.0f, 0.0f, 1.0f), // identity quaternion
          cmd_w(Vector3f::Zero()), cmd_v(Vector3f::Zero()), cmd_acc(Vector3f::Zero())
    {
    }
};
} // namespace lark::drones