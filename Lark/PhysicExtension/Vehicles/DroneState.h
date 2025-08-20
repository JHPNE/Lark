#include "Utils/MathTypes.h"

namespace lark::drones {

    struct DroneState {

        math::v3 position;
        math::v3 velocity;
        math::v4 attitude;
        math::v3 body_rates;
        math::v3 wind;
        math::v4 rotor_speeds;
    };

    enum class ControlAbstraction {
        /// @brief Direct motor speed control (lowest level)
        CMD_MOTOR_SPEEDS,

        /// @brief Individual rotor thrust commands
        CMD_MOTOR_THRUSTS,

        /// @brief Collective thrust + body angular rates
        CMD_CTBR,

        /// @brief Collective thrust + body moments
        CMD_CTBM,

        /// @brief Collective thrust + attitude quaternion
        CMD_CTATT,

        /// @brief Velocity vector in world frame
        CMD_VEL,

        /// @brief Acceleration vector in world frame (mass-normalized thrust)
        CMD_ACC
    };
}