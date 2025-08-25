#include "Controller.h"

namespace lark::drones {
 ControlInput Control::computeMotorCommands(const DroneState& state,
                                      const TrajectoryPoint& desired) const {
     ControlInput input = ControlInput();

     Vector3f pos_err = state.position - desired.position;
     Vector3f dpos_err = state.velocity - desired.velocity;

     Vector3f neg_kp_pos = -m_dynamics.GetQuadParams().control_gains.kp_pos;
     Vector3f term1 = neg_kp_pos.cwiseProduct(pos_err);

     Vector3f neg_kd_pos = -m_dynamics.GetQuadParams().control_gains.kd_pos;
     Vector3f term2 = neg_kd_pos.cwiseProduct(dpos_err);
     Vector3f term3 = {0, 0, 9.81f};
     Vector3f subterm = desired.acceleration + term3;
     auto combinedTerm = term1 + term2 + subterm;

     Vector3f F_des = m_dynamics.GetQuadParams().inertia_properties.mass * combinedTerm;

     // Get current body z-axis in world frame
     Matrix3f R = quaternionToRotationMatrix(state.attitude);
     Vector3f b3 = R.col(2);  // Third column of R

     // Project desired force onto current thrust direction
     float u1 = F_des.dot(b3);  // Collective thrust command

     // Orientation
     Vector3f b3_des = F_des.normalized();
     float yaw_des = desired.yaw;
     Vector3f c1_des(std::cos(yaw_des), std::sin(yaw_des), 0);
     Vector3f b2_des = b3_des.cross(c1_des).normalized();
     Vector3f b1_des = b2_des.cross(b3_des);

     Matrix3f R_des;
     R_des.col(0) = b1_des;
     R_des.col(1) = b2_des;
     R_des.col(2) = b3_des;
     R_des.transposeInPlace();

     // Orientation error
     Matrix3f test = R_des.transpose() * R;
     Matrix3f test2 = R.transpose() * R_des;
     Matrix3f t = test - test2;
     Matrix3f S_err = 0.5f * t;
     Vector3f att_err = veeMap(S_err);

     // Angular velocity error
     Vector3f w_des(0, 0, desired.yaw_dot);
     Vector3f w_err = state.body_rates - w_des;

     // Desired torque
     Vector3f u2 = m_dynamics.GetInertiaMatrix() *
            (-m_dynamics.GetQuadParams().control_gains.kp_att * att_err
             - m_dynamics.GetQuadParams().control_gains.kd_att * w_err) +
            state.body_rates.cross(m_dynamics.GetInertiaMatrix() * state.body_rates);

     Vector3f cmd_w = -m_dynamics.GetQuadParams().control_gains.kp_att * att_err
                         - m_dynamics.GetQuadParams().control_gains.kd_att * w_err;

     Vector4f TM(u1, u2.x(), u2.y(), u2.z());
     Matrix4f TM_to_f = m_dynamics.GetInverseControlAllocationMatrix();
     Vector4f cmd_rotor_thrust = TM_to_f.transpose() * TM;
     Vector4f cmd_motor_speeds = cmd_rotor_thrust / m_dynamics.GetQuadParams().rotor_properties.k_eta;
     cmd_motor_speeds = cmd_motor_speeds.cwiseSign().cwiseProduct(cmd_motor_speeds.cwiseAbs().cwiseSqrt());

     input.cmd_motor_speeds = cmd_motor_speeds;
     input.cmd_motor_thrusts = cmd_rotor_thrust;
     input.cmd_thrust = u1;
     input.cmd_moment = u2;
     input.cmd_w = cmd_w;
     input.cmd_q = rotationMatrixToQuaternion(R_des.transpose());
     auto b = -m_dynamics.GetQuadParams().control_gains.kp_vel.cwiseProduct(pos_err);
     input.cmd_v = b + desired.velocity;
     input.cmd_acc = F_des / m_dynamics.GetQuadParams().inertia_properties.mass;
     return input;
 };
}
