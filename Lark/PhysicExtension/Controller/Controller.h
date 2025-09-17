#pragma once
#include "PhysicExtension/Trajectory/Trajectory.h"
#include "PhysicExtension/Utils/DroneDynamics.h"

namespace lark::drones
{
class Control
{
  public:
    explicit Control(const QuadParams &quad_params)
        : m_dynamics(quad_params)
    {
    }

    [[nodiscard]] ControlInput computeMotorCommands(const DroneState &state,
                                                    const TrajectoryPoint &desired) const;

  private:
    DroneDynamics m_dynamics;
};
} // namespace lark::drones