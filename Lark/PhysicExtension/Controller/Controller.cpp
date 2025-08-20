#include "Controller.h"
#include <glm/gtx/quaternion.hpp>


namespace lark::drones {
 ControlInput Control::computeMotorCommands(const DroneState& state,
                                      const TrajectoryPoint& desired) const {
     ControlInput input = ControlInput();

     math::v3 pos_err = state.position - desired.position;
     math::v3 dpos_err = state.velocity - desired.velocity;

     math::v3 F_des = m_dynamics.GetQuadParams().inertia_properties.mass *
         (-m_dynamics.GetQuadParams().control_gains.kp_pos * pos_err
             - m_dynamics.GetQuadParams().control_gains.kd_pos * dpos_err
             + desired.acceleration + math::v3(0,0,9.81f));

     // Get current body z-axis in world frame
     math::m3x3 R = math::quaternionToRotationMatrix(state.attitude);
     math::v3 b3 = R * math::v3(0, 0, 1);  // Third column of R

     // Project desired force onto current thrust direction
     float u1 = glm::dot(F_des, b3);  // This is our collective thrust command

     // orientation
     math::v3 b3_des = math::normalize(F_des);
     float yaw_des = desired.yaw;
     math::v3 c1_des = {glm::cos(yaw_des), glm::sin(yaw_des), 0};
     math::v3 b2_des = math::normalize(glm::cross(b3_des, c1_des));
     math::v3 b1_des = glm::cross(b2_des, b3_des);
     math::m3x3 R_des = glm::transpose(math::m3x3(b1_des, b2_des, b3_des));

     // Orientation error
     math::m3x3 S_err = 0.5f * (glm::transpose(R_des) * R - glm::transpose(R) * R_des);
     math::v3 att_err = math::vee_map(S_err);

     // Angular velocity error
     math::v3 w_des = {0, 0, desired.yaw_dot};
     math::v3 w_err = state.angular_velocity - w_des;

     // Desired torque
     math::v3 u2 = m_dynamics.GetInertiaMatrix() *
         (m_dynamics.GetQuadParams().control_gains.kp_att * att_err - m_dynamics.GetQuadParams().control_gains.kd_att * w_err) +
             glm::cross(state.angular_velocity, m_dynamics.GetInertiaMatrix() * state.angular_velocity);

     math::v3 cmd_w = -m_dynamics.GetQuadParams().control_gains.kp_att * att_err - m_dynamics.GetQuadParams().control_gains.kd_att * w_err;

     math::v4 TM = {u1, u2[0], u2[1], u2[2]};
     math::v4 cmd_rotor_thrust = m_dynamics.GetInverseControlAllocationMatrix() * TM;
     math::v4 cmd_motor_speeds = cmd_rotor_thrust / m_dynamics.GetQuadParams().rotor_properties.k_eta;
     cmd_motor_speeds = glm::sign(cmd_motor_speeds) * glm::sqrt(glm::abs(cmd_motor_speeds));

     input.cmd_thrust = u1;
     input.cmd_moment = u2;
     input.cmd_w = cmd_w;
     input.cmd_q = math::rotationMatrixToQuaternion(R_des);
     input.cmd_v = -m_dynamics.GetQuadParams().control_gains.kp_vel*pos_err + desired.velocity;
     input.cmd_acc = F_des/m_dynamics.GetQuadParams().inertia_properties.mass;

     return input;
 };
}
