#include <gtest/gtest.h>
#include "Physics/Multirotor.h"
#include <cmath>
#include <memory>

namespace lark::test {

/**
 * @brief Test constants with explicit type definitions for precision
 */
namespace {
    constexpr float kEpsilon = 1e-6f;              // Tolerance for floating point comparisons
    constexpr float kTimeStep = 0.01f;             // Simulation time step (s)
    constexpr float kGravity = 9.81f;              // Gravitational acceleration (m/s^2)
    constexpr float kArmLength = 0.25f;            // Quadrotor arm length (m)
}

/**
 * @brief Standard quad configuration for testing
 * @details Provides consistent quadrotor parameters for test reproducibility
 */
struct QuadrotorConfig {
    static drones::InertiaProperties GetInertialProps() {
        drones::InertiaProperties props;
        props.mass = 1.0f;                         // Mass in kg
        props.Ixx = 1.23e-2f;                      // Moment of inertia (kg⋅m²)
        props.Iyy = 1.23e-2f;
        props.Izz = 1.23e-2f;
        props.Ixy = 0.0f;
        props.Iyz = 0.0f;
        props.Ixz = 0.0f;
        return props;
    }

    static drones::AerodynamicProperties GetAeroProps() {
        drones::AerodynamicProperties props;
        props.dragCoeffX = 0.1f;
        props.dragCoeffY = 0.1f;
        props.dragCoeffZ = 0.1f;
        props.enableAerodynamics = false;          // Disable aero for basic tests
        return props;
    }

    static drones::MotorProperties GetMotorProps() {
        drones::MotorProperties props;
        props.responseTime = 0.02f;                // Motor response time (s)
        props.noiseStdDev = 0.0f;                 // Disable noise for deterministic tests
        props.bodyRateGain = 1.0f;
        props.velocityGain = 2.0f;
        props.attitudePGain = 5.0f;
        props.attitudeDGain = 1.0f;
        return props;
    }

    static std::vector<drones::RotorParameters> GetRotorProps() {
        std::vector<drones::RotorParameters> rotors(4);

        // Front right (CCW)
        rotors[0].position = glm::vec3(kArmLength, -kArmLength, 0.0f);
        rotors[0].direction = 1;

        // Front left (CW)
        rotors[1].position = glm::vec3(kArmLength, kArmLength, 0.0f);
        rotors[1].direction = -1;

        // Rear left (CCW)
        rotors[2].position = glm::vec3(-kArmLength, kArmLength, 0.0f);
        rotors[2].direction = 1;

        // Rear right (CW)
        rotors[3].position = glm::vec3(-kArmLength, -kArmLength, 0.0f);
        rotors[3].direction = -1;

        // Common rotor parameters
        for (auto& rotor : rotors) {
            rotor.thrustCoeff = 1e-5f;            // N/(rad/s)^2
            rotor.torqueCoeff = 1e-6f;            // Nm/(rad/s)^2
            rotor.dragCoeff = 1e-4f;              // N/(rad*m/s^2)
            rotor.inflowCoeff = 1e-4f;            // N/(rad*m/s^2)
            rotor.flapCoeff = 1e-5f;              // Nm/(rad*m/s^2)
            rotor.minSpeed = 0.0f;                // rad/s
            rotor.maxSpeed = 1000.0f;             // rad/s
        }

        return rotors;
    }
};

/**
 * @brief Test fixture for Multirotor testing
 * @details Provides standardized setup and helper functions for tests
 */
class MultirotorTest : public ::testing::Test {
protected:
    void SetUp() override {
        try {
            multirotor = std::make_unique<drones::Multirotor>(
                QuadrotorConfig::GetInertialProps(),
                QuadrotorConfig::GetAeroProps(),
                QuadrotorConfig::GetMotorProps(),
                QuadrotorConfig::GetRotorProps()
            );

            // Initialize default hover state
            defaultState.position = glm::vec3(0.0f);
            defaultState.velocity = glm::vec3(0.0f);
            defaultState.orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            defaultState.angular_velocity = glm::vec3(0.0f);
            defaultState.wind = glm::vec3(0.0f);
            defaultState.rotor_speeds = std::vector<float>(4, 100.0f);

            // Initialize default control input
            defaultControl.mode = drones::ControlMode::MOTOR_SPEEDS;
            defaultControl.motorSpeeds = std::vector<float>(4, 100.0f);
        }
        catch (const std::exception& e) {
            FAIL() << "Failed to create Multirotor: " << e.what();
        }
    }

    void TearDown() override {
        multirotor.reset();
    }

    /**
     * @brief Verify vector components are approximately equal
     * @param actual Actual vector
     * @param expected Expected vector
     * @param epsilon Maximum allowed difference
     */
    static void ExpectVec3Near(
        const glm::vec3& actual,
        const glm::vec3& expected,
        float epsilon = kEpsilon) {
        EXPECT_NEAR(actual.x, expected.x, epsilon);
        EXPECT_NEAR(actual.y, expected.y, epsilon);
        EXPECT_NEAR(actual.z, expected.z, epsilon);
    }

    std::unique_ptr<drones::Multirotor> multirotor;
    drones::DroneState defaultState;
    drones::ControlInput defaultControl;
};

/**
 * @brief Verify basic drone construction
 */
TEST_F(MultirotorTest, Construction) {
    EXPECT_EQ(multirotor->getRotorCount(), 4);
    EXPECT_EQ(multirotor->getControlMode(), drones::ControlMode::MOTOR_SPEEDS);
}

/**
 * @brief Verify free-fall acceleration under gravity
 */
TEST_F(MultirotorTest, FreeFall) {
    // Set zero thrust
    defaultControl.motorSpeeds = std::vector<float>(4, 0.0f);

    // Step simulation
    auto currentState = multirotor->step(defaultState, defaultControl, kTimeStep);

    // Verify only vertical acceleration due to gravity
    EXPECT_NEAR(currentState.velocity.z, -kGravity * kTimeStep, kEpsilon);
    EXPECT_NEAR(currentState.velocity.x, 0.0f, kEpsilon);
    EXPECT_NEAR(currentState.velocity.y, 0.0f, kEpsilon);
}

/**
 * @brief Verify basic derivatives computation
 */
TEST_F(MultirotorTest, BasicDerivatives) {
    auto [linearAccel, angularAccel] = multirotor->computeStateDerivatives(
        defaultState, defaultControl, kTimeStep);

    // With no thrust, expect only gravity in z
    EXPECT_NEAR(linearAccel.z, -kGravity, kEpsilon);

    // With symmetric thrust, expect no angular acceleration
    ExpectVec3Near(angularAccel, glm::vec3(0.0f));
}

} // namespace lark::test