#pragma once
#include "PhysicExtension/Utils/DroneDynamics.h"
#include "PhysicExtension/Trajectory/Trajectory.h"

namespace lark::drones {
    class Control{
    public:
        explicit Control(const QuadParams& quad_params,
                           const ControlGains& gains = ControlGains())
            : m_dynamics(quad_params),
              m_gains(gains) {
        }

        [[nodiscard]] ControlInput computeMotorCommands(const DroneState& state,
                                      const TrajectoryPoint& desired) const;

    private:
        DroneDynamics m_dynamics;
        ControlGains m_gains;

    };
}