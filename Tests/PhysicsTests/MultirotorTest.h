#pragma once
#include "Physics/Multirotor.h"
#include <cmath>
#include <gtest/gtest.h>
#include <memory>

namespace lark::test {
using namespace drones;

namespace {
  // Test constants with explicit type definitions
  constexpr float kEpsilon = 1e-6f;
  constexpr float kTimeStep = 0.01f;
  constexpr float kGravity = 9.81f;

  // Standard quadrotor configuration for testing
  struct QuadrotorConfig {
    static InertiaProperties GetInertialProps() {
      InertiaProperties props;
      props.mass = 1.0f;
      props.Ixx = 0.0123f;
      props.Iyy = 0.0123f;
      props.Izz = 0.0123f;
      props.Ixy = 0.0f;
      props.Iyz = 0.0f;
      props.Ixz = 0.0f;
      return props;
    }

    static AerodynamicProperties GetAeroProps() {
      AerodynamicProperties props;
      props.dragCoeffX = 0.1f;
      props.dragCoeffY = 0.1f;
      props.dragCoeffZ = 0.1f;
      props.enableAerodynamics = true;
      return props;
    }

    static MotorProperties GetMotorProps() {
      MotorProperties props;
      props.responseTime = 0.02f;
      props.noiseStdDev = 0.0f;
      props.bodyRateGain = 1.0f;
      props.velocityGain = 2.0f;
      props.attitudePGain = 5.0f;
      props.attitudeDGain = 1.0f;
      return props;
    }

    static std::vector<RotorParameters> GetRotorProps() {
      // Standard X configuration quadrotor
      std::vector<RotorParameters> rotors(4);
      const float arm_length = 0.25f;

      // Front right - CCW
      rotors[0].position = glm::vec3(arm_length, -arm_length, 0.0f);
      rotors[0].direction = 1;

      // Front left - CW
      rotors[1].position = glm::vec3(arm_length, arm_length, 0.0f);
      rotors[1].direction = -1;

      // Rear left - CCW
      rotors[2].position = glm::vec3(-arm_length, arm_length, 0.0f);
      rotors[2].direction = 1;

      // Rear right - CW
      rotors[3].position = glm::vec3(-arm_length, -arm_length, 0.0f);
      rotors[3].direction = -1;

      // Common parameters
      for (auto& rotor : rotors) {
        rotor.thrustCoeff = 1e-5f;
        rotor.torqueCoeff = 1e-6f;
        rotor.dragCoeff = 1e-4f;
        rotor.inflowCoeff = 1e-4f;
        rotor.flapCoeff = 1e-5f;
        rotor.minSpeed = 0.0f;
        rotor.maxSpeed = 1000.0f;
      }

      return rotors;
    }
  };
}

class MultirotorTest : public ::testing::Test {
protected:
    void SetUp() override {
        try {
            multirotor = std::make_unique<Multirotor>(
                QuadrotorConfig::GetInertialProps(),
                QuadrotorConfig::GetAeroProps(),
                QuadrotorConfig::GetMotorProps(),
                QuadrotorConfig::GetRotorProps()
            );
        } catch (const std::exception& e) {
            FAIL() << "Failed to create Multirotor: " << e.what();
        }

        // Initialize standard test state
        defaultState.position = glm::vec3(0.0f);
        defaultState.velocity = glm::vec3(0.0f);
        defaultState.orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
        defaultState.angular_velocity = glm::vec3(0.0f);
        defaultState.wind = glm::vec3(0.0f);
        defaultState.rotor_speeds = std::vector<float>(4, 100.0f);

        // Initialize standard control input
        defaultControl.mode = ControlMode::MOTOR_SPEEDS;
        defaultControl.motorSpeeds = std::vector<float>(4, 100.0f);
    }

    void TearDown() override {
        multirotor.reset();
    }

    // Utility function to check if vectors are approximately equal
    static void ExpectVec3Near(const glm::vec3& actual, const glm::vec3& expected, float epsilon = kEpsilon) {
        EXPECT_NEAR(actual.x, expected.x, epsilon);
        EXPECT_NEAR(actual.y, expected.y, epsilon);
        EXPECT_NEAR(actual.z, expected.z, epsilon);
    }

    // Utility function to check if quaternions are approximately equal
    static void ExpectQuatNear(const glm::quat& actual, const glm::quat& expected, float epsilon = kEpsilon) {
        // Account for q and -q representing same rotation
        float dot = std::abs(
            actual.w * expected.w +
            actual.x * expected.x +
            actual.y * expected.y +
            actual.z * expected.z
        );
        EXPECT_NEAR(dot, 1.0f, epsilon);
    }

    std::unique_ptr<Multirotor> multirotor;
    DroneState defaultState;
    ControlInput defaultControl;
};

TEST_F(MultirotorTest, Construction) {
    EXPECT_EQ(multirotor->getRotorCount(), 4);
    EXPECT_EQ(multirotor->getControlMode(), ControlMode::MOTOR_SPEEDS);
}

TEST_F(MultirotorTest, StateValidation) {
    // Test valid state
    EXPECT_FALSE(multirotor->validateState(defaultState));

    // Test invalid rotor count
    DroneState invalidState = defaultState;
    invalidState.rotor_speeds.pop_back();
    EXPECT_TRUE(multirotor->validateState(invalidState));
}

TEST_F(MultirotorTest, ControlValidation) {
    // Test valid control
    EXPECT_FALSE(multirotor->validateControl(defaultControl));

    // Test invalid motor count
    ControlInput invalidControl = defaultControl;
    invalidControl.motorSpeeds.pop_back();
    EXPECT_TRUE(multirotor->validateControl(invalidControl));
}

TEST_F(MultirotorTest, HoverStability) {
    // Configure hover state with thrust matching gravity
    const float hover_speed = std::sqrt((kGravity * QuadrotorConfig::GetInertialProps().mass) /
                                      (4.0f * QuadrotorConfig::GetRotorProps()[0].thrustCoeff));

    DroneState hoverState = defaultState;
    ControlInput hoverControl = defaultControl;
    hoverControl.motorSpeeds = std::vector<float>(4, hover_speed);

    // Simulate for 1 second
    constexpr int steps = static_cast<int>(1.0f / kTimeStep);
    DroneState currentState = hoverState;

    for (int i = 0; i < steps; ++i) {
        currentState = multirotor->step(currentState, hoverControl, kTimeStep);
    }

    // Check position maintained within tolerance
    // Slight downwards force still due to body wrench
    ExpectVec3Near(currentState.position, hoverState.position,1.0f);
    ExpectVec3Near(currentState.velocity, hoverState.velocity, 0.4f);
    ExpectQuatNear(currentState.orientation, hoverState.orientation);
    ExpectVec3Near(currentState.angular_velocity, hoverState.angular_velocity, 0.01f);
}

TEST_F(MultirotorTest, DerivativeComputation) {
    auto [linearAccel, angularAccel] = multirotor->computeStateDerivatives(
        defaultState, defaultControl, kTimeStep);

    // Check gravity appears in acceleration
    EXPECT_NEAR(linearAccel.z, -kGravity, 1);

    // With symmetric configuration and equal thrusts, should have no angular acceleration
    ExpectVec3Near(angularAccel, glm::vec3(0.0f));
}

TEST_F(MultirotorTest, ControlModeTransitions) {
    // Test all valid mode transitions
    std::vector<ControlMode> modes = {
        ControlMode::MOTOR_SPEEDS,
        ControlMode::MOTOR_THRUSTS,
        ControlMode::COLLECTIVE_THRUST_BODY_RATES,
        ControlMode::COLLECTIVE_THRUST_BODY_MOMENTS,
        ControlMode::COLLECTIVE_THRUST_ATTITUDE,
        ControlMode::VELOCITY
    };

    for (const auto& mode : modes) {
        EXPECT_NO_THROW(multirotor->setControlMode(mode));
        EXPECT_EQ(multirotor->getControlMode(), mode);
    }
}

TEST_F(MultirotorTest, InvalidInputHandling) {
    // Test invalid timestep
    EXPECT_THROW(multirotor->step(defaultState, defaultControl, -1.0f), std::invalid_argument);

    // Test invalid state
    DroneState invalidState = defaultState;
    invalidState.rotor_speeds.clear();
    EXPECT_THROW(multirotor->step(invalidState, defaultControl, kTimeStep), std::invalid_argument);

    // Test invalid control
    ControlInput invalidControl = defaultControl;
    invalidControl.motorSpeeds.clear();
    EXPECT_THROW(multirotor->step(defaultState, invalidControl, kTimeStep), std::invalid_argument);
}

}
