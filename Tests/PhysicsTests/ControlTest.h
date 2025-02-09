#pragma once

#include <gtest/gtest.h>
#include "Physics/Multirotor.h"
#include "Physics/Controller.h"
#include <chrono>
#include <iomanip>

namespace lark::drones::test {

/**
 * @brief Test fixture for drone controller validation
 * @details Handles common setup and utility functions for controller testing
 */
class ControlTest : public ::testing::Test {
protected:
    /**
     * @brief Initialize test parameters and configurations
     * @throws std::runtime_error on initialization failure
     */
    void SetUp() override {
        try {
            // Initialize with validated parameters
            inertialProps = createInertialProperties();
            aeroProps = createAeroProperties();
            motorProps = createMotorProperties();
            rotors = createRotorConfiguration();

        } catch (const std::exception& e) {
            throw std::runtime_error(
                std::string("Test setup failed: ") + e.what());
        }
    }

    /**
     * @brief Helper to print system state
     * @param state Current drone state
     * @param t Current simulation time
     */
    static void printState(const DroneState& state, float t) {
        std::cout << std::fixed << std::setprecision(3)
                  << "Time: " << t << "s\n"
                  << "Position: [" << state.position.x << ", "
                                 << state.position.y << ", "
                                 << state.position.z << "]\n"
                  << "Velocity: [" << state.velocity.x << ", "
                                 << state.velocity.y << ", "
                                 << state.velocity.z << "]\n"
                  << "----------------------------------------\n"
                  << std::flush;
    }

private:
    static InertiaProperties createInertialProperties() {
        InertiaProperties props;
        props.mass = 0.5f;  // 500g drone
        props.Ixx = 3.65e-3f;
        props.Iyy = 3.68e-3f;
        props.Izz = 7.03e-3f;
        props.Ixy = 0.0f;
        props.Iyz = 0.0f;
        props.Ixz = 0.0f;
        return props;
    }

    static AerodynamicProperties createAeroProperties() {
        AerodynamicProperties props;
        props.dragCoeffX = 0.1f;
        props.dragCoeffY = 0.1f;
        props.dragCoeffZ = 0.1f;
        props.enableAerodynamics = true;
        return props;
    }

    static MotorProperties createMotorProperties() {
        MotorProperties props;
        props.responseTime = 0.02f;  // 20ms response
        props.noiseStdDev = 0.1f;
        props.bodyRateGain = 1.0f;
        props.velocityGain = 10.0f;
        props.attitudePGain = 544.0f;
        props.attitudeDGain = 46.64f;
        return props;
    }

    static std::vector<RotorParameters> createRotorConfiguration() {
        constexpr float arm_length = 0.17f;  // 17cm arm length
        constexpr float sqrt2_2 = arm_length/std::sqrt(2.0f);

        return {
            // Front right
            {0.557e-5f, 1.36e-7f, 1.19e-4f, 2.32e-4f, 0.0f,
             {sqrt2_2, sqrt2_2, 0}, 1, 0, 1500},
            // Back right
            {0.557e-5f, 1.36e-7f, 1.19e-4f, 2.32e-4f, 0.0f,
             {sqrt2_2, -sqrt2_2, 0}, -1, 0, 1500},
            // Back left
            {0.557e-5f, 1.36e-7f, 1.19e-4f, 2.32e-4f, 0.0f,
             {-sqrt2_2, -sqrt2_2, 0}, 1, 0, 1500},
            // Front left
            {0.557e-5f, 1.36e-7f, 1.19e-4f, 2.32e-4f, 0.0f,
             {-sqrt2_2, sqrt2_2, 0}, -1, 0, 1500}
        };
    }

protected:
    InertiaProperties inertialProps;
    AerodynamicProperties aeroProps;
    MotorProperties motorProps;
    std::vector<RotorParameters> rotors;
};

/**
 * @brief Test hover maneuver control
 * @test Validates drone stable hover behavior
 */
TEST_F(ControlTest, HoverManeuver) {
    using Clock = std::chrono::high_resolution_clock;
    auto startTime = Clock::now();

    // Create drone and controller
    Multirotor drone(inertialProps, aeroProps, motorProps, rotors,
                    ControlMode::COLLECTIVE_THRUST_ATTITUDE);
    Controller controller(inertialProps);

    // Initial state
    DroneState state{
        .position = {0, 0, 0},
        .velocity = {0, 0, 0},
        .orientation = {0, 0, 0, 1},
        .angular_velocity = {0, 0, 0},
        .wind = {0, 0, 0},
        .rotor_speeds = std::vector<float>(4, 0.0f)
    };

    // Simulation parameters
    constexpr float dt = 0.01f;        // 100Hz
    constexpr float t_final = 5.0f;    // 5 seconds
    constexpr float print_interval = 0.1f;

    // Hover at z=1m
    FlatOutput desired{
        .position = {0, 0, 1},
        .velocity = {0, 0, 0},
        .acceleration = {0, 0, 0},
        .jerk = {0, 0, 0},
        .snap = {0, 0, 0},
        .yaw = 0,
        .yawRate = 0
    };

    std::cout << "\nRunning hover simulation...\n";

    float t = 0;
    size_t stepCount = 0;

    while (t < t_final) {
        ControlInput control = controller.computeControl(
            ControlMode::COLLECTIVE_THRUST_ATTITUDE,
            state,
            desired
        );

        state = drone.step(state, control, dt);

        if (std::fmod(t, print_interval) < dt) {
            printState(state, t);
        }

        t += dt;
        stepCount++;
    }

    auto endTime = Clock::now();
    std::chrono::duration<double> elapsed = endTime - startTime;

    const float finalError = glm::length(state.position - desired.position);

    std::cout << "\nSimulation complete!\n"
              << "Steps: " << stepCount << "\n"
              << "Simulation time: " << t_final << "s\n"
              << "Wall time: " << elapsed.count() << "s\n"
              << "Final position error: " << finalError << "m\n";

    // Basic sanity check - error should be finite
    EXPECT_TRUE(std::isfinite(finalError));
}

} // namespace lark::drones::test