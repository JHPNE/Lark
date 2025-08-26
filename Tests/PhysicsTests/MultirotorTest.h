#include <gtest/gtest.h>
#include "PhysicExtension/Vehicles/Multirotor.h"
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