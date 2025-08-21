#include "Multirotor.h"

namespace lark::drones {
    math::v4 Multirotor::GetCMDMotorSpeeds(DroneState state, ControlInput input) {
        float cmd_thrust;
        math::v3 cmd_moment, att_err, F_des;
        math::m3x3 R, R_des, S_err;
        switch (m_control_abstraction) {
            case ControlAbstraction::CMD_MOTOR_SPEEDS:
                return input.cmd_motor_speeds;

            case ControlAbstraction::CMD_MOTOR_THRUSTS:
                math::v4 motor_speeds = input.cmd_motor_thrusts/m_dynamics.GetQuadParams().rotor_properties.k_eta;
                return glm::sign(motor_speeds) * glm::sqrt(glm::abs(motor_speeds));

            case ControlAbstraction::CMD_CTBM:
                cmd_thrust = input.cmd_thrust;
                cmd_moment = input.cmd_moment;

            case ControlAbstraction::CMD_CTBR:
                cmd_thrust = input.cmd_thrust;

                // First compute the error between the desired body rates and the actual body rates given by state.
                math::v3 w_err = state.body_rates - input.cmd_w;

                math::v3 w_dot_cmd = -m_dynamics.GetQuadParams().lower_level_controller_properties.k_w * w_err;

                cmd_moment = m_dynamics.GetInertiaMatrix() * w_dot_cmd;
            case ControlAbstraction::CMD_VEL:
                math::v3 v_err = state.velocity - input.cmd_v;
                math::v3 a_cmd = -m_dynamics.GetQuadParams().lower_level_controller_properties.k_v * v_err;

                F_des = m_dynamics.GetQuadParams().inertia_properties.mass * (a_cmd + math::v3(0, 0, 9.81f));

                R = math::quaternionToRotationMatrix(state.attitude);
                math::v3 b3 = R * math::v3(0, 0, 1);
                cmd_thrust = glm::dot(F_des, b3);

                math::v3 b3_des = glm::normalize(F_des);
                math::v3 c1_des(1.0f, 0.0f, 0.0f);  // Fixed forward direction
                math::v3 b2_des = glm::normalize(glm::cross(b3_des, c1_des));
                math::v3 b1_des = glm::cross(b2_des, b3_des);

                R_des = glm::transpose(math::m3x3(b1_des, b2_des, b3_des));


                // Orientation error
                S_err = 0.5f * (glm::transpose(R_des) * R - glm::transpose(R) * R_des);
                att_err = math::vee_map(S_err);


                cmd_moment = m_dynamics.GetInertiaMatrix() *
                    (-m_dynamics.GetQuadParams().control_gains.kp_att*att_err - m_dynamics.GetQuadParams().control_gains.kd_att*state.body_rates) +
                        glm::cross(state.body_rates, m_dynamics.GetInertiaMatrix() * state.body_rates);

            case ControlAbstraction::CMD_CTATT:

                cmd_thrust = input.cmd_thrust;

                R = math::quaternionToRotationMatrix(state.attitude);
                R_des = math::quaternionToRotationMatrix(input.cmd_q);

                S_err = 0.5f * (glm::transpose(R_des) * R - glm::transpose(R) * R_des);
                att_err = math::vee_map(S_err);

                cmd_moment = m_dynamics.GetInertiaMatrix() *
                    (-m_dynamics.GetQuadParams().control_gains.kp_att * att_err - m_dynamics.GetQuadParams().control_gains.kd_att*state.body_rates) +
                    glm::cross(state.body_rates, m_dynamics.GetInertiaMatrix() * state.body_rates);

            case ControlAbstraction::CMD_ACC:
                F_des = input.cmd_acc * m_dynamics.GetInertiaMatrix();
                R = math::quaternionToRotationMatrix(state.attitude);
                b3 = R * math::v3(0, 0, 1);
                cmd_thrust = glm::dot(F_des, b3);

                b3_des = math::normalize(F_des);
                c1_des = math::v3(1, 0, 0);
                b2_des = math::normalize(glm::cross(b3_des, c1_des));
                b1_des = glm::cross(b2_des, b3_des);

                R_des = glm::transpose(math::m3x3(b1_des, b2_des, b3_des));


                S_err = 0.5f * (glm::transpose(R_des) * R - glm::transpose(R) * R_des);
                att_err = math::vee_map(S_err);

                cmd_moment = m_dynamics.GetInertiaMatrix() *
                    (-m_dynamics.GetQuadParams().control_gains.kp_att * att_err - m_dynamics.GetQuadParams().control_gains.kd_att*state.body_rates) +
                    glm::cross(state.body_rates, m_dynamics.GetInertiaMatrix() * state.body_rates);
        }

        math::v4 TM(input.cmd_thrust, cmd_moment.x, cmd_moment.y, cmd_moment.z);
        math::v4 cmd_motor_forces = m_dynamics.GetInverseControlAllocationMatrix() * TM;
        math::v4 cmd_motor_speeds = cmd_motor_forces / m_dynamics.GetQuadParams().rotor_properties.k_eta;
        cmd_motor_speeds = glm::sign(cmd_motor_speeds) * glm::sqrt(glm::abs(cmd_motor_speeds));

        return cmd_motor_speeds;

    }

    void Multirotor::ComputeBodyWrench(math::v3 body_rate, math::v4 rotor_speeds, math::v3 body_airspeed_vector) {
        // Fix for local_airspeeds (3x4 matrix - 3 components for 4 rotors)
        glm::mat3x4 local_airspeeds = math::hatMap(body_rate) * glm::transpose(m_dynamics.GetRotorGeometry());

        // Add body_airspeed_vector to each COLUMN (each rotor)
        for (int i = 0; i < 4; ++i) {  // 4 rotors, not 3
            local_airspeeds[i] += body_airspeed_vector;
        }

        // Compute T matrix - thrust vector for each rotor
        // T = [0, 0, k_eta]^T * rotor_speeds^2 for each rotor
        glm::mat3x4 T(0.0f);  // Initialize to zeros
        float k_eta = m_dynamics.GetQuadParams().rotor_properties.k_eta;

        for (int i = 0; i < 4; ++i) {
            // Only z-component is non-zero (thrust is along rotor axis)
            T[i].z = k_eta * rotor_speeds[i] * rotor_speeds[i];
            // T[i].x = 0;  // Already zero from initialization
            // T[i].y = 0;  // Already zero from initialization
        }    }


    void Multirotor::s_dot_fn(DroneState state, math::v4 cmd_rotor_speeds) {
        math::v4 rotor_speeds = state.rotor_speeds;
        math::v3 inertia_velocity = state.velocity;
        math::v3 wind_velocity = state.wind;

        math::m3x3 R = math::quaternionToRotationMatrix(state.attitude);

        math::v4 rotor_accel = (1/m_dynamics.GetQuadParams().motor_properties.tau_m) * (cmd_rotor_speeds - rotor_speeds);

        math::v3 x_dot = state.velocity;

        glm::quat q_dot = math::quat_dot(glm::quat(state.attitude), state.body_rates);

        math::v3 body_airspeed_vector = glm::transpose(R) * (inertia_velocity - wind_velocity);





    }


    void Multirotor::step(DroneState state, ControlInput input, float dt) {
        math::v3 cmd_rotor_speeds = GetCMDMotorSpeeds(state, input);
        cmd_rotor_speeds = clamp(cmd_rotor_speeds, m_dynamics.GetQuadParams().motor_properties.rotor_speed_min, m_dynamics.GetQuadParams().motor_properties.rotor_speed_max);
    }

}