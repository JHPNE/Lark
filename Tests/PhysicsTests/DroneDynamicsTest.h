#include <gtest/gtest.h>
#include "PhysicExtension/Utils/DroneDynamics.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <cmath>

namespace lark::drones::test {

    class MultirotorTest : public ::testing::Test {
    protected:
        // Helper to create AscTec Hummingbird parameters
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
                math::v3{ d * sqrt2_2,  d * sqrt2_2, 0.0f},  // Front-right
                math::v3{ d * sqrt2_2, -d * sqrt2_2, 0.0f},  // Back-right
                math::v3{-d * sqrt2_2, -d * sqrt2_2, 0.0f},  // Back-left
                math::v3{-d * sqrt2_2,  d * sqrt2_2, 0.0f}   // Front-left
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

        bool matrix_near(const glm::mat4& a, const glm::mat4& b, float tolerance = 1e-5f) {
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    if (std::abs(a[i][j] - b[i][j]) > tolerance) {
                        return false;
                    }
                }
            }
            return true;
        }

        // Helper function to compare vec3 vectors
        bool vec3_near(const math::v3& a, const math::v3& b, float tolerance = 1e-5f) {
            return std::abs(a.x - b.x) < tolerance &&
                   std::abs(a.y - b.y) < tolerance &&
                   std::abs(a.z - b.z) < tolerance;
        }

        // Alternative: Use this macro for more detailed failure messages
        void EXPECT_VEC3_NEAR(const math::v3& actual, const math::v3& expected, float tolerance = 1e-5f) {
            EXPECT_NEAR(actual.x, expected.x, tolerance) << "X component mismatch";
            EXPECT_NEAR(actual.y, expected.y, tolerance) << "Y component mismatch";
            EXPECT_NEAR(actual.z, expected.z, tolerance) << "Z component mismatch";
        }

        // Helper for vec4 comparisons (for rotor speeds, quaternions)
        void EXPECT_VEC4_NEAR(const math::v4& actual, const math::v4& expected, float tolerance = 1e-5f) {
            EXPECT_NEAR(actual.x, expected.x, tolerance) << "X component mismatch";
            EXPECT_NEAR(actual.y, expected.y, tolerance) << "Y component mismatch";
            EXPECT_NEAR(actual.z, expected.z, tolerance) << "Z component mismatch";
            EXPECT_NEAR(actual.w, expected.w, tolerance) << "W component mismatch";
        }

        // Helper for mat3x3 comparisons
        void EXPECT_MAT3_NEAR(const math::m3x3& actual, const math::m3x3& expected, float tolerance = 1e-5f) {
            for (int i = 0; i < 3; ++i) {
                for (int j = 0; j < 3; ++j) {
                    EXPECT_NEAR(actual[i][j], expected[i][j], tolerance)
                        << "Matrix element [" << i << "][" << j << "] mismatch";
                }
            }
        }
    };

    TEST_F(MultirotorTest, ConstructorInitialization) {
        QuadParams params = createHummingbirdParams();

        DroneDynamics drone_dynamics(params);

        // Test weight calculation
        math::v3 expected_weight = {0.0f, 0.0f, -0.500f * 9.81f};
        EXPECT_VEC3_NEAR(drone_dynamics.GetWeight(), expected_weight, 1e-5f);

        // Test torque-thrust ratio
        float expected_ratio = params.rotor_properties.k_m / params.rotor_properties.k_eta;
        EXPECT_NEAR(expected_ratio, 1.36e-07f / 5.57e-06f, 1e-10f);
    }

    TEST_F(MultirotorTest, InertiaMatrixVerification) {
        QuadParams params = createHummingbirdParams();

        // Test the inertia matrix construction
        math::m3x3 inertia = params.inertia_properties.GetInertiaMatrix();

        // Expected inertia matrix for Hummingbird (diagonal since products are zero)
        math::m3x3 expected = {
            3.65e-3f, 0.0f, 0.0f,
            0.0f, 3.68e-3f, 0.0f,
            0.0f, 0.0f, 7.03e-3f
        };

        EXPECT_MAT3_NEAR(inertia, expected, 1e-10f);
    }

    TEST_F(MultirotorTest, DragMatrixVerification) {
        QuadParams params = createHummingbirdParams();

        math::m3x3 drag_matrix = params.aero_dynamics_properties.GetDragMatrix();

        math::m3x3 expected = {
            0.5e-2f, 0.0f, 0.0f,
            0.0f, 0.5e-2f, 0.0f,
            0.0f, 0.0f, 1e-2f
        };

        EXPECT_MAT3_NEAR(drag_matrix, expected, 1e-10f);
    }

    TEST_F(MultirotorTest, ControlAllocationMatrixStructure) {
        QuadParams params = createHummingbirdParams();

        DroneDynamics drone_dynamics(params);

        // The expected f_to_TM matrix should be:
        // Column 0 (Rotor 1): [1, y1, -x1, k_m/k_eta * dir1]
        // Column 1 (Rotor 2): [1, y2, -x2, k_m/k_eta * dir2]
        // Column 2 (Rotor 3): [1, y3, -x3, k_m/k_eta * dir3]
        // Column 3 (Rotor 4): [1, y4, -x4, k_m/k_eta * dir4]

        math::m4x4 expected_f_to_TM = drone_dynamics.GetControlAllocationMatrix();

        // Verify specific values for Hummingbird configuration
        const float d = 0.17f;
        const float sqrt2_2 = 0.70710678118f;

        // Check thrust row (should be all 1s)
        EXPECT_FLOAT_EQ(expected_f_to_TM[0][0], 1.0f);
        EXPECT_FLOAT_EQ(expected_f_to_TM[1][0], 1.0f);
        EXPECT_FLOAT_EQ(expected_f_to_TM[2][0], 1.0f);
        EXPECT_FLOAT_EQ(expected_f_to_TM[3][0], 1.0f);

        // Check roll moments
        EXPECT_NEAR(expected_f_to_TM[0][1], d * sqrt2_2, 1e-6f);  // Rotor 1
        EXPECT_NEAR(expected_f_to_TM[1][1], -d * sqrt2_2, 1e-6f); // Rotor 2
        EXPECT_NEAR(expected_f_to_TM[2][1], -d * sqrt2_2, 1e-6f); // Rotor 3
        EXPECT_NEAR(expected_f_to_TM[3][1], d * sqrt2_2, 1e-6f);  // Rotor 4
    }

}
