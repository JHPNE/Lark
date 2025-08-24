#include <gtest/gtest.h>
#include "PhysicExtension/Controller/Controller.h"
#include "PhysicExtension/Utils/DroneState.h"

namespace lark::drones::test {
    using namespace physics_math;

    class ControllerTest : public ::testing::Test {
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
    };

    TEST_F(ControllerTest, StateInitialization) {
        DroneState state = createState();
    };
}
