#pragma once
#include <gtest/gtest.h>
#include "PhysicExtension/Vehicles/Multirotor.h"
#include "PhysicExtension/Controller/Controller.h"
#include "PhysicExtension/Utils/DroneState.h"

namespace lark::drones::test {
    using namespace physics_math;

    class MultirotorTest : public ::testing::Test {
    protected:
        QuadParams createHummingbirdParams() {
            QuadParams params;

            // Inertia properties
            params.inertia_properties.mass = 0.500f;
            params.inertia_properties.principal_inertia = {3.65e-3f, 3.68e-3f, 7.03e-3f};
            params.inertia_properties.product_inertia = {0.0f, 0.0f, 0.0f};

            // Geometric properties
            const float d = 0.17f; // Arm length
            const float sqrt2_2 = 0.70710678118f; // sqrt(2)/2

            params.geometric_properties.rotor_radius = 0.10f;
            params.geometric_properties.rotor_positions = {
                Vector3f{ d * sqrt2_2,  d * sqrt2_2, 0.0f},  // Front-right
                Vector3f{ d * sqrt2_2, -d * sqrt2_2, 0.0f},  // Back-right
                Vector3f{-d * sqrt2_2, -d * sqrt2_2, 0.0f},  // Back-left
                Vector3f{-d * sqrt2_2,  d * sqrt2_2, 0.0f}   // Front-left
            };
            params.geometric_properties.rotor_directions = {1, -1, 1, -1};
            params.geometric_properties.imu_position = {0.0f, 0.0f, 0.0f};

            // Aerodynamics properties
            params.aero_dynamics_properties.parasitic_drag = {0.5e-2f, 0.5e-2f, 1e-2f};

            // Rotor properties
            params.rotor_properties.k_eta = 5.57e-06f;
            params.rotor_properties.k_m = 1.36e-07f;
            params.rotor_properties.k_d = 1.19e-04f;
            params.rotor_properties.k_z = 2.32e-04f;
            params.rotor_properties.k_h = 3.39e-3f;
            params.rotor_properties.k_flap = 0.0f;

            // Motor properties
            params.motor_properties.tau_m = 0.005f;
            params.motor_properties.rotor_speed_min = 0.0f;
            params.motor_properties.rotor_speed_max = 1500.0f;
            params.motor_properties.motor_noise_std = 0.0f;

            // Controller properties
            params.lower_level_controller_properties.k_w = 1;
            params.lower_level_controller_properties.k_v = 10;
            params.lower_level_controller_properties.kp_att = 544;
            params.lower_level_controller_properties.kd_att = 46.64f;

            return params;
        }

        DroneState createState() {
            DroneState state{};
            state.position = Vector3f::Zero();
            state.velocity = Vector3f::Zero();
            state.attitude = Vector4f(0, 0, 0, 1);
            state.body_rates = Vector3f::Zero();
            state.wind = Vector3f::Zero();
            state.rotor_speeds = Vector4f(1788.53, 1788.53, 1788.53, 1788.53);

            return state;
        };

        TrajectoryPoint createTrajectoryPoint() {
            TrajectoryPoint point{};
            point.position = Vector3f::Zero();
            point.velocity = Vector3f(1, 1, 0);
            point.acceleration = Vector3f::Zero();
            point.jerk = Vector3f(-1, -1, 0);
            point.snap = Vector3f::Zero();

            point.yaw = 0;
            point.yaw_dot = 0;
            point.yaw_ddot = 0;

            return point;
        }

        // Helper function to compare vectors with tolerance
        void EXPECT_VEC3_NEAR(const Vector3f& actual, const Vector3f& expected, float tolerance = 1e-4f) {
            EXPECT_NEAR(actual.x(), expected.x(), tolerance) << "X component mismatch";
            EXPECT_NEAR(actual.y(), expected.y(), tolerance) << "Y component mismatch";
            EXPECT_NEAR(actual.z(), expected.z(), tolerance) << "Z component mismatch";
        }

        void EXPECT_VEC4_NEAR(const Vector4f& actual, const Vector4f& expected, float tolerance = 1e-4f) {
            EXPECT_NEAR(actual(0), expected(0), tolerance) << "Component 0 mismatch";
            EXPECT_NEAR(actual(1), expected(1), tolerance) << "Component 1 mismatch";
            EXPECT_NEAR(actual(2), expected(2), tolerance) << "Component 2 mismatch";
            EXPECT_NEAR(actual(3), expected(3), tolerance) << "Component 3 mismatch";
        }
    };

    TEST_F(MultirotorTest, StateDotTestHummingbird) {
        QuadParams params = createHummingbirdParams();
        Control controller(params);

        DroneState state = createState();
        TrajectoryPoint point = createTrajectoryPoint();

        ControlInput result = controller.computeMotorCommands(state, point);

        Multirotor multirotor(params, state, ControlAbstraction::CMD_MOTOR_SPEEDS);
        StateDot actual = multirotor.stateDot(state, result, 0);

        Eigen::Vector3f vdot = {0, 0, 132.73029083};
        Eigen::Vector3f wdot = {0.0f, -9.694455805196546e-14f, 0.0f};

        EXPECT_VEC3_NEAR(actual.vdot, vdot);
        EXPECT_VEC3_NEAR(actual.wdot, wdot);
    }

    TEST_F(MultirotorTest, StepTestHummingbird) {
        QuadParams params = createHummingbirdParams();
        Control controller(params);

        DroneState state = createState();
        state.wind = Vector3f(0.06279051952931337, 0.06279051952931337, 0.06279051952931337);
        TrajectoryPoint point = createTrajectoryPoint();

        ControlInput result = controller.computeMotorCommands(state, point);

        Multirotor multirotor(params, state, ControlAbstraction::CMD_MOTOR_SPEEDS);
        DroneState result1 = multirotor.step(state, result, 0.01);

        // rk45 integration
        /*
        Vector3f expected_position{4.842575037441486e-06, 4.8968090090213734e-06, 0.003229048386634305};
        Vector3f expected_velocity{0.0010416192568423845, 0.0010608577542681695, 0.4795714816709453};
        Vector4f expected_attitude{-0.002022944724344893, 0.001813244200240739, -0.0005004203071620471, 0.9999961847025362};  // Quaternion [x,y,z,w]
        Vector3f expected_body_rates{-0.8692547060514457, 0.7704085190757337, -0.22636673873388363}; // w
        Vector3f expected_wind{0.06279051952931337, 0.06279051952931337, 0.06279051952931337};
        Vector4f expected_rotor_speeds{242.1086173485404, 968.3892283485751, 696.7811089428872, 951.0528204785043};
        */
        // eigen integration
        Vector3f expected_position{0, 0, 0};
        Vector3f expected_velocity{0.0010698048564464313, 0.0010698048564464313, 1.3293907512335472};
        Vector4f expected_attitude{0.0, 0.0, 0.0, 1.0};  // Quaternion [x,y,z,w]
        Vector3f expected_body_rates{0.0, -9.010715667577915e-16, 6.873376671188568e-20}; // w
        Vector3f expected_wind{0.06279051952931337, 0.06279051952931337, 0.06279051952931337};
        Vector4f expected_rotor_speeds{0.0, 0.0, 0.0, 0.0};

        EXPECT_VEC3_NEAR(result1.position, expected_position);
        EXPECT_VEC3_NEAR(result1.velocity, expected_velocity);
        EXPECT_VEC4_NEAR(result1.attitude, expected_attitude);
        EXPECT_VEC3_NEAR(result1.body_rates, expected_body_rates);
        EXPECT_VEC3_NEAR(result1.wind, expected_wind);
        EXPECT_VEC4_NEAR(result1.rotor_speeds, expected_rotor_speeds);
    }
}