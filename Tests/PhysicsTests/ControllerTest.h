#pragma once
#include "PhysicExtension/Controller/Controller.h"
#include "PhysicExtension/Utils/DroneState.h"
#include "CSVParser.h"
#include "PhysicExtension/Vehicles/Multirotor.h"

#include <gtest/gtest.h>

namespace lark::drone::test
{
using namespace physics_math;

class ControllerTest : public ::testing::Test
{
  protected:
    QuadParams createHummingbirdParams()
    {
        QuadParams params;

        // Inertia properties
        params.inertia_properties.mass = 0.500f;
        params.inertia_properties.principal_inertia = {3.65e-3f, 3.68e-3f, 7.03e-3f};
        params.inertia_properties.product_inertia = {0.0f, 0.0f, 0.0f};

        // Geometric properties
        const float d = 0.17f;                // Arm length
        const float sqrt2_2 = 0.70710678118f; // sqrt(2)/2

        params.geometric_properties.rotor_radius = 0.10f;
        params.geometric_properties.rotor_positions = {
            Vector3f{d * sqrt2_2, d * sqrt2_2, 0.0f},   // Front-right
            Vector3f{d * sqrt2_2, -d * sqrt2_2, 0.0f},  // Back-right
            Vector3f{-d * sqrt2_2, -d * sqrt2_2, 0.0f}, // Back-left
            Vector3f{-d * sqrt2_2, d * sqrt2_2, 0.0f}   // Front-left
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

    DroneState createState()
    {
        DroneState state{};
        state.position = Vector3f::Zero();
        state.velocity = Vector3f::Zero();
        state.attitude = Vector4f(0, 0, 0, 1);
        state.body_rates = Vector3f::Zero();
        state.wind = Vector3f::Zero();
        state.rotor_speeds = Vector4f(1788.53, 1788.53, 1788.53, 1788.53);

        return state;
    };

    TrajectoryPoint createTrajectoryPoint()
    {
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
    void EXPECT_VEC3_NEAR(const Vector3f &actual, const Vector3f &expected, float tolerance = 1e-4f)
    {
        EXPECT_NEAR(actual.x(), expected.x(), tolerance) << "X component mismatch";
        EXPECT_NEAR(actual.y(), expected.y(), tolerance) << "Y component mismatch";
        EXPECT_NEAR(actual.z(), expected.z(), tolerance) << "Z component mismatch";
    }

    void EXPECT_VEC4_NEAR(const Vector4f &actual, const Vector4f &expected, float tolerance = 1e-4f)
    {
        EXPECT_NEAR(actual(0), expected(0), tolerance) << "Component 0 mismatch";
        EXPECT_NEAR(actual(1), expected(1), tolerance) << "Component 1 mismatch";
        EXPECT_NEAR(actual(2), expected(2), tolerance) << "Component 2 mismatch";
        EXPECT_NEAR(actual(3), expected(3), tolerance) << "Component 3 mismatch";
    }

    void compareStates(const DroneState& actual,
                      const CSVParser::SimulationData& expected,
                      const std::string& label,
                      float pos_tol = 1e-3f,
                      float vel_tol = 1e-3f,
                      float quat_tol = 1e-4f,
                      float rate_tol = 1e-3f) {

        EXPECT_NEAR(actual.position.x(), expected.position.x(), pos_tol)
            << label << " - Position X mismatch";
        EXPECT_NEAR(actual.position.y(), expected.position.y(), pos_tol)
            << label << " - Position Y mismatch";
        EXPECT_NEAR(actual.position.z(), expected.position.z(), pos_tol)
            << label << " - Position Z mismatch";

        EXPECT_NEAR(actual.velocity.x(), expected.velocity.x(), vel_tol)
            << label << " - Velocity X mismatch";
        EXPECT_NEAR(actual.velocity.y(), expected.velocity.y(), vel_tol)
            << label << " - Velocity Y mismatch";
        EXPECT_NEAR(actual.velocity.z(), expected.velocity.z(), vel_tol)
            << label << " - Velocity Z mismatch";

        // Quaternion comparison (handle possible sign flip)
        float dot = actual.attitude.dot(expected.quaternion);
        if (std::abs(dot) < 0.999f) {  // Not close enough
            EXPECT_NEAR(std::abs(dot), 1.0f, quat_tol)
                << label << " - Quaternion mismatch";
        }

        EXPECT_NEAR(actual.body_rates.x(), expected.body_rates.x(), rate_tol)
            << label << " - Body rate X mismatch";
        EXPECT_NEAR(actual.body_rates.y(), expected.body_rates.y(), rate_tol)
            << label << " - Body rate Y mismatch";
        EXPECT_NEAR(actual.body_rates.z(), expected.body_rates.z(), rate_tol)
            << label << " - Body rate Z mismatch";
    }
};

TEST_F(ControllerTest, StateInitialization)
{
    DroneState state = createState();
    EXPECT_EQ(state.position, Vector3f::Zero());
    EXPECT_EQ(state.velocity, Vector3f::Zero());
};

TEST_F(ControllerTest, ControllerOutputVerificationHummingbird)
{
    QuadParams params = createHummingbirdParams();
    Control controller(params);

    DroneState state = createState();
    TrajectoryPoint point = createTrajectoryPoint();

    ControlInput result = controller.computeMotorCommands(state, point);

    // Expected values
    Vector3f expected_cmd_acc(4.0f, 4.0f, 9.81f);
    Vector3f expected_cmd_moment(-0.72553277, 0.68096356, -0.25501914);
    Vector4f expected_cmd_motor_speeds(-879.65333314, 839.98751942, 525.85627728, 819.93691361);
    Vector4f expected_cmd_motor_thrusts(-4.31001022, 3.93007521, 1.54024327, 3.74469174);
    Vector4f expected_cmd_q(-0.18925107, 0.17617741, -0.03453756, 0.96537698);
    float expected_cmd_thrust = 4.905f;
    Vector3f expected_cmd_v(1, 1, 0);
    Vector3f expected_cmd_w(-198.77610163, 185.04444606, -36.27583788);

    // Verify results
    EXPECT_VEC3_NEAR(result.cmd_acc, expected_cmd_acc, 1e-3f);
    EXPECT_VEC3_NEAR(result.cmd_moment, expected_cmd_moment, 1e-5f);

    // Note: cmd_motor_speeds and cmd_motor_thrusts are not directly returned by
    // computeMotorCommands They would need to be computed or exposed through the controller if
    // needed
    EXPECT_VEC4_NEAR(result.cmd_q, expected_cmd_q, 1e-4f);
    EXPECT_NEAR(result.cmd_thrust, expected_cmd_thrust, 1e-4f);
    EXPECT_VEC3_NEAR(result.cmd_v, expected_cmd_v, 1e-4f);
    EXPECT_VEC3_NEAR(result.cmd_w, expected_cmd_w, 1e-2f);
}

TEST_F(ControllerTest, CompareWithPythonSimulation) {
    // Load reference data from Python simulation
    auto reference_data = CSVParser::parseCSV("/Users/am/CLionProjects/Lark/basic_usage.csv");
    ASSERT_FALSE(reference_data.empty()) << "Failed to load reference data";

    // Debug: print first few data points
    std::cout << "First data point:" << std::endl;
    std::cout << "  Time: " << reference_data[0].time << std::endl;
    std::cout << "  Position: " << reference_data[0].position.transpose() << std::endl;
    std::cout << "  Velocity: " << reference_data[0].velocity.transpose() << std::endl;

    // Setup simulation with initial conditions from Python data
    QuadParams params = createHummingbirdParams();

    DroneState state;
    state.position = reference_data[0].position;
    state.velocity = reference_data[0].velocity;
    state.attitude = reference_data[0].quaternion;
    state.body_rates = reference_data[0].body_rates;
    state.wind = reference_data[0].wind;
    state.rotor_speeds = reference_data[0].rotor_speeds;

    // Create vehicle and controller
    lark::drone::Multirotor vehicle(params, state, ControlAbstraction::CMD_MOTOR_SPEEDS);
    Control controller(params);

    // Assuming the Python simulation used a specific trajectory, recreate it
    // You might need to adjust this based on your Python simulation
    auto trajectory = std::make_shared<Circular>(
        Vector3f::Zero(),  // Use desired position as center
        1.0f, 0.2f, false
    );

    const float dt = 0.01f;  // Match Python timestep

    // Compare at each timestep
    for (size_t i = 1; i < reference_data.size(); ++i) {
        float t = reference_data[i].time;

        // Create trajectory point from reference data
        TrajectoryPoint desired;
        desired.position = reference_data[i].position_des;
        desired.velocity = reference_data[i].velocity_des;
        desired.acceleration = reference_data[i].acceleration_des;
        desired.jerk = reference_data[i].jerk_des;
        desired.snap = reference_data[i].snap_des;
        desired.yaw = reference_data[i].yaw_des;
        desired.yaw_dot = reference_data[i].yaw_dot_des;

        // Compute control
        ControlInput control = controller.computeMotorCommands(state, desired);

        // Verify control outputs match Python
        EXPECT_NEAR(control.cmd_thrust, reference_data[i].cmd_thrust, 0.1f)
            << "Thrust mismatch at t=" << t;

        for (int j = 0; j < 3; ++j) {
            EXPECT_NEAR(control.cmd_moment[j], reference_data[i].cmd_moment[j], 0.01f)
                << "Moment[" << j << "] mismatch at t=" << t;
        }

        // Step dynamics
        state = vehicle.step(state, control, dt);

        // Compare state with Python
        std::string label = "t=" + std::to_string(t);
        compareStates(state, reference_data[i], label);

        // Optional: stop early if diverging too much
        Vector3f pos_error = state.position - reference_data[i].position;
        if (pos_error.norm() > 1.0f) {
            FAIL() << "Simulation diverged from Python at t=" << t
                   << ", position error: " << pos_error.norm();
        }
    }
}
} // namespace lark::drones::test
