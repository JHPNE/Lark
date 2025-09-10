#include "Multirotor.h"
#include <random>
#include "PhysicExtension/World/WorldSettings.h"

namespace lark::drones {
    Vector4f Multirotor::GetCMDMotorSpeeds(DroneState state, ControlInput input) {
        float cmd_thrust;
        Vector3f cmd_moment, att_err, F_des, b3, c1_des;
        Matrix3f R, R_des, S_err;

        switch (m_control_abstraction) {
            case ControlAbstraction::CMD_MOTOR_SPEEDS:
                return input.cmd_motor_speeds;

            case ControlAbstraction::CMD_MOTOR_THRUSTS: {
                Vector4f motor_speeds = input.cmd_motor_thrusts/m_dynamics.GetQuadParams().rotor_properties.k_eta;
                return motor_speeds.cwiseSign().cwiseProduct(motor_speeds.cwiseAbs().cwiseSqrt());
            }

            case ControlAbstraction::CMD_CTBM:
                cmd_thrust = input.cmd_thrust;
                cmd_moment = input.cmd_moment;
                break;  // ADD BREAK

            case ControlAbstraction::CMD_CTBR: {  // ADD BRACES
                cmd_thrust = input.cmd_thrust;
                Vector3f w_err = state.body_rates - input.cmd_w;
                Vector3f w_dot_cmd = -m_dynamics.GetQuadParams().lower_level_controller_properties.k_w * w_err;
                cmd_moment = m_dynamics.GetInertiaMatrix() * w_dot_cmd;
                break;  // ADD BREAK
            }

            case ControlAbstraction::CMD_VEL: {  // ADD BRACES
                Vector3f v_err = state.velocity - input.cmd_v;
                Vector3f a_cmd = -m_dynamics.GetQuadParams().lower_level_controller_properties.k_v * v_err;

                Vector3f subterm = a_cmd + Vector3f(0, 0, 9.81f);

                F_des = m_dynamics.GetQuadParams().inertia_properties.mass * subterm;

                R = quaternionToRotationMatrix(state.attitude);
                b3 = R.col(2);
                cmd_thrust = F_des.dot(b3);

                Vector3f b3_des = F_des.normalized();
                c1_des = Vector3f(1.0f, 0.0f, 0.0f);
                Vector3f b2_des = b3_des.cross(c1_des).normalized();
                Vector3f b1_des = b2_des.cross(b3_des);

                R_des.col(0) = b1_des;
                R_des.col(1) = b2_des;
                R_des.col(2) = b3_des;
                R_des.transposeInPlace();

                Matrix3f subMatrix =  R_des.transpose() * R - R.transpose() * R_des;
                S_err = 0.5f * (subMatrix);
                att_err = veeMap(S_err);

                cmd_moment = GetCMDMoment(state, att_err);
                break;  // ADD BREAK
            }

            case ControlAbstraction::CMD_CTATT: {  // ADD BRACES
                cmd_thrust = input.cmd_thrust;
                R = quaternionToRotationMatrix(state.attitude);
                R_des = quaternionToRotationMatrix(input.cmd_q);

                Matrix3f subMatrix = R_des.transpose() * R - R.transpose() * R_des;
                S_err = 0.5f * (subMatrix);
                att_err = veeMap(S_err);

                cmd_moment = GetCMDMoment(state, att_err);
                break;
            }

            case ControlAbstraction::CMD_ACC: {  // ADD BRACES
                F_des = input.cmd_acc * m_dynamics.GetQuadParams().inertia_properties.mass;
                R = quaternionToRotationMatrix(state.attitude);
                b3 = R.col(2);
                cmd_thrust = F_des.dot(b3);

                Vector3f b3_des = F_des.normalized();
                c1_des = Vector3f(1.0f, 0.0f, 0.0f);
                Vector3f b2_des = b3_des.cross(c1_des).normalized();
                Vector3f b1_des = b2_des.cross(b3_des);

                R_des.col(0) = b1_des;
                R_des.col(1) = b2_des;
                R_des.col(2) = b3_des;
                R_des.transposeInPlace();

                Matrix3f subMatrix = R_des.transpose() * R - R.transpose() * R_des;
                S_err = 0.5f * (subMatrix);
                att_err = veeMap(S_err);

                cmd_moment = GetCMDMoment(state, att_err);
                break;
            }

            default:
                cmd_thrust = 0.0f;
                cmd_moment = Vector3f::Zero();
                break;
        }

        Vector4f TM(cmd_thrust, cmd_moment.x(), cmd_moment.y(), cmd_moment.z());
        Vector4f cmd_motor_forces = m_dynamics.GetInverseControlAllocationMatrix() * TM;
        Vector4f cmd_motor_speeds = cmd_motor_forces / m_dynamics.GetQuadParams().rotor_properties.k_eta;
        cmd_motor_speeds = cmd_motor_speeds.cwiseSign().cwiseProduct(cmd_motor_speeds.cwiseAbs().cwiseSqrt());

        return cmd_motor_speeds;
    }

    std::pair<Vector3f, Vector3f> Multirotor::ComputeBodyWrench(const Vector3f& body_rate, Vector4f rotor_speeds, const Vector3f& body_airspeed_vector) {
        // Compute local airspeeds (3x4 matrix - 3 components for 4 rotors)
        // We need 3x4 for the multiplication, so transpose it
        // In NumPy: (n,1) * (m,) broadcasts → (n,m)
        // In Eigen: VectorN * VectorM.transpose() → (n,m)
        auto geometry_transposed = m_dynamics.GetRotorGeometry().transpose();

        Eigen::Matrix<float, 3, Eigen::Dynamic> local_airspeeds(3, geometry_transposed.cols());

        auto replicated_airspeed = body_airspeed_vector.replicate(1, geometry_transposed.cols()).cast<float>();
        auto rotational_velocity = hatMap(body_rate) * geometry_transposed;
        local_airspeeds =  replicated_airspeed + rotational_velocity;

        // rotor speeds square
        Eigen::Vector4f rotor_square = rotor_speeds.array().square();

        Eigen::Vector3f Tvec(0,0,m_dynamics.GetQuadParams().rotor_properties.k_eta);
        Eigen::Matrix<float, 3,4> T = Tvec * rotor_square.transpose();

        Vector3f D = Vector3f::Zero();
        Eigen::Matrix3Xf H = Eigen::Matrix3Xf::Zero(3, rotor_speeds.size());
        Eigen::Matrix3Xf M_flap = Eigen::Matrix3Xf::Zero(3, rotor_speeds.size());

        if (m_aero) {
            float airspeed_magnitude = body_airspeed_vector.norm();
            Matrix3f drag_matrix = m_dynamics.GetQuadParams().aero_dynamics_properties.GetDragMatrix();
            Vector3f drag_direction = drag_matrix * body_airspeed_vector;
            D = -airspeed_magnitude * drag_direction;

            // H force calculation
            Eigen::Matrix3Xf temp = m_dynamics.GetQuadParams().rotor_properties.GetRotorDragMatrix() * local_airspeeds;
            H = -temp.array() * rotor_speeds.transpose().replicate(3,1).array();

            // Pitching flapping moment acting at each propeller hub
            Vector3f z_unit(0, 0, 1);
            for (int i = 0; i < m_dynamics.GetQuadParams().geometric_properties.num_rotors; ++i) {
                Matrix3f hat_local = hatMap(local_airspeeds.col(i));
                M_flap.col(i) = -m_dynamics.GetQuadParams().rotor_properties.k_flap * rotor_speeds(i) * (hat_local * z_unit);
            }

            // Translational lift
            Eigen::RowVectorXf xy_squared = local_airspeeds.topRows<2>().colwise().squaredNorm();
            T.row(2).array() += m_dynamics.GetQuadParams().rotor_properties.k_h * xy_squared.array();
        }

        // Compute the moments due to the rotor thrusts, rotor drag, and rotor drag torques
        Vector3f M_force = Vector3f::Zero();
        for (int i = 0; i < m_dynamics.GetQuadParams().geometric_properties.num_rotors; ++i) {
            Vector3f r = geometry_transposed.col(i);
            Vector3f f = T.col(i) + H.col(i);
            Vector3f c = hatMap(r) * f;
            M_force += c;
        }

        M_force = -M_force;


        Eigen::Vector3f subterm(0, 0, m_dynamics.GetQuadParams().rotor_properties.k_m);
        Eigen::Vector4f rotor_dir    = m_dynamics.GetQuadParams().geometric_properties.rotor_directions;

        // scale each column j by rotor_square[j] * rotor_dir[j]
        Eigen::Vector4f col_scale = rotor_square.cwiseProduct(rotor_dir);

        // (3x4) result
        Eigen::Matrix<float,3,4> M_yaw = subterm * col_scale.transpose();

        // Sum all elements to compute the total body wrench
        Vector3f thrust_sum = T.rowwise().sum();
        Vector3f h_force_sum = H.rowwise().sum();
        Vector3f a = (thrust_sum + h_force_sum);
        Vector3f FtotB =  a + D;

        Vector3f yaw_moment_sum = M_yaw.rowwise().sum();
        Vector3f flap_moment_sum = M_flap.rowwise().sum();
        Vector3f b = M_force + yaw_moment_sum;
        Vector3f MtotB = b + flap_moment_sum;

        return std::make_pair(FtotB, MtotB);
    }

    SDot Multirotor::s_dot_fn(DroneState state, Vector4f cmd_rotor_speeds) {
        Vector4f rotor_speeds = state.rotor_speeds;
        Vector3f inertia_velocity = state.velocity;
        Vector3f wind_velocity = state.wind;

        Matrix3f R = quaternionToRotationMatrix(state.attitude);

        // rotor speeds derivative
        float tau_scalar = 1.0f/m_dynamics.GetQuadParams().motor_properties.tau_m;
        Vector4f rotor_diff = cmd_rotor_speeds - rotor_speeds;
        Vector4f rotor_accel = tau_scalar * rotor_diff;

        // position derivative
        Vector3f x_dot = state.velocity;

        // orientation derivative
        Vector4f q_dot = quatDot(state.attitude, state.body_rates);

        Vector3f velocity_diff = inertia_velocity - wind_velocity;

        Vector3f body_airspeed_vector = R.transpose() * velocity_diff;

        std::pair<Vector3f, Vector3f> pairs = ComputeBodyWrench(state.body_rates, rotor_speeds, body_airspeed_vector);
        Vector3f FtotB = pairs.first;
        Vector3f MtotB = pairs.second;

        // will be used directly in bullet
        Ftot = R * FtotB;
        Mtot = R * MtotB;

        if (m_enable_ground and state.position.y() == 0.f) {
            Ftot -= m_dynamics.GetWeight();
        }

        // velocity derivative
        Vector3f v_dot = (m_dynamics.GetWeight() + Ftot) / m_dynamics.GetQuadParams().inertia_properties.mass;

        Vector3f wind_dot = Vector3f::Zero();

        // Angular velocity derivative
        Vector3f w = state.body_rates;
        Matrix3f w_hat = hatMap(w);
        Vector3f inertia_body_rates = m_dynamics.GetInertiaMatrix() * w;
        Vector3f test = w_hat * inertia_body_rates;

        Vector3f test2 = MtotB - test;
        Vector3f w_dot = m_dynamics.GetInverseInertia() * test2;

        return {x_dot, v_dot, q_dot, w_dot, wind_dot, rotor_accel};
    }


    DroneState Multirotor::step(DroneState state, ControlInput input, float dt) {
        Vector4f cmd_rotor_speeds = GetCMDMotorSpeeds(state, input);

        // Clamp rotor speeds
        cmd_rotor_speeds = cmd_rotor_speeds.cwiseMax(m_dynamics.GetQuadParams().motor_properties.rotor_speed_min)
                                           .cwiseMin(m_dynamics.GetQuadParams().motor_properties.rotor_speed_max);

        // Compute state derivative
        SDot s_dot = s_dot_fn(state, cmd_rotor_speeds);

        // Euler integration - update each component directly
        state.position += s_dot.xdot * dt;
        state.velocity += s_dot.vdot * dt;
        state.body_rates += s_dot.wdot * dt;
        state.wind += s_dot.wind_dot * dt;
        state.rotor_speeds += s_dot.rotor_accel * dt;

        // Quaternion integration requires special handling
        // Convert quaternion derivative to integration
        state.attitude += s_dot.qdot * dt;

       // Re-normalize quaternion
        state.attitude.normalize();

        // Add noise to motor speeds (if motor_noise > 0)
        if (m_dynamics.GetQuadParams().motor_properties.motor_noise_std > 0) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<float> noise(0.0f, std::abs(m_dynamics.GetQuadParams().motor_properties.motor_noise_std));

            for (int i = 0; i < state.rotor_speeds.size(); ++i) {
                state.rotor_speeds(i) += noise(gen);
            }
        }

        // Clamp rotor speeds after noise
        state.rotor_speeds = state.rotor_speeds.cwiseMax(m_dynamics.GetQuadParams().motor_properties.rotor_speed_min)
                                               .cwiseMin(m_dynamics.GetQuadParams().motor_properties.rotor_speed_max);

        return state;
    }

    StateDot Multirotor::stateDot(DroneState state, ControlInput input, float dt) {
        Vector4f cmd_motor_speeds = GetCMDMotorSpeeds(state, input);
        Vector4f cmd_rotor_speeds = cmd_motor_speeds.cwiseMax(m_dynamics.GetQuadParams().motor_properties.rotor_speed_min).cwiseMin(m_dynamics.GetQuadParams().motor_properties.rotor_speed_max);

        SDot s_dot = s_dot_fn(state, cmd_rotor_speeds);

        Eigen::VectorXf v_dot = s_dot.vdot;  // Extract elements 3, 4, 5
        Eigen::VectorXf w_dot = s_dot.wdot; // Extract elements 10, 11, 12


        return {v_dot, w_dot};
    }


}