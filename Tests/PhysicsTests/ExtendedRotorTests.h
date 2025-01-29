#pragma once
#include "RotorPhysicsTest.h"
#include <random>
#include <array>

namespace lark::tests {

class ExtendedRotorTests : public RotorPhysicsTest {
protected:
    void SetUp() override {
        RotorPhysicsTest::SetUp();
        setupRandomGenerator();
    }

    void setupRandomGenerator() {
        std::random_device rd;
        m_rng = std::mt19937(rd());
    }

    float getRandomFloat(float min, float max) {
        std::uniform_real_distribution<float> dist(min, max);
        return dist(m_rng);
    }

    void configureRandomRotor() {
        m_rotorData.bladeRadius = getRandomFloat(0.1f, 0.4f);  // 10-40cm radius
        m_rotorData.bladePitch = getRandomFloat(0.1f, 0.3f);   // ~5.7-17.2 degrees
        m_rotorData.bladeCount = static_cast<int>(getRandomFloat(2.0f, 6.99f));  // 2-6 blades
        m_rotorData.mass = getRandomFloat(0.05f, 0.3f);        // 50-300g
        m_rotorData.discArea = models::PI * m_rotorData.bladeRadius * m_rotorData.bladeRadius;

        // Recreate rigid body with new parameters
        if (m_rotorBody) {
            m_dynamicsWorld->removeRigidBody(m_rotorBody);
            delete m_rotorBody->getMotionState();
            delete m_rotorBody->getCollisionShape();
            delete m_rotorBody;
        }

        btCollisionShape* shape = new btCylinderShape(
            btVector3(m_rotorData.bladeRadius, 0.02f, m_rotorData.bladeRadius));

        btTransform transform;
        transform.setIdentity();
        transform.setOrigin(btVector3(0, 1, 0));

        btVector3 localInertia(0, 0, 0);
        shape->calculateLocalInertia(m_rotorData.mass, localInertia);

        btDefaultMotionState* motionState = new btDefaultMotionState(transform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(
            m_rotorData.mass, motionState, shape, localInertia);

        m_rotorBody = new btRigidBody(rbInfo);
        m_rotorBody->setDamping(0.1f, 0.1f);
        m_dynamicsWorld->addRigidBody(m_rotorBody);

        m_rotorData.rigidBody = m_rotorBody;
        m_rotorData.dynamics_world = m_dynamicsWorld.get();

        rotor::physics::initialize_blade_properties(&m_rotorData);
        rotor::physics::initialize_motor_parameters(&m_rotorData);
    }

    models::AtmosphericConditions getRandomAtmosphericConditions() {
        float altitude = getRandomFloat(0.0f, 3000.0f);  // 0-3000m
        float velocity = getRandomFloat(0.0f, 30.0f);    // 0-30 m/s
        return models::calculate_atmospheric_conditions(altitude, velocity);
    }

private:
    std::mt19937 m_rng;
};

// Test scaling laws with different rotor configurations
TEST_F(ExtendedRotorTests, ScalingLawsTest) {
    const std::array<float, 3> test_rpms = {3000.0f, 6000.0f, 9000.0f};

    for (int i = 0; i < 10; ++i) {  // Test 10 different configurations
        configureRandomRotor();
        auto conditions = getRandomAtmosphericConditions();

        std::array<float, 3> thrusts;
        std::array<float, 3> powers;

        for (size_t j = 0; j < test_rpms.size(); ++j) {
            m_rotorData.currentRPM = test_rpms[j];
            thrusts[j] = rotor::physics::calculate_thrust(&m_rotorData, conditions);
            powers[j] = rotor::physics::calculate_power(&m_rotorData, thrusts[j], conditions);
        }

        // Verify thrust scales with square of RPM
        float thrust_ratio_1 = thrusts[1] / thrusts[0];
        float thrust_ratio_2 = thrusts[2] / thrusts[0];
        float rpm_ratio_1_squared = std::pow(test_rpms[1] / test_rpms[0], 2);
        float rpm_ratio_2_squared = std::pow(test_rpms[2] / test_rpms[0], 2);

        EXPECT_NEAR(thrust_ratio_1, rpm_ratio_1_squared, rpm_ratio_1_squared * 0.15f);
        EXPECT_NEAR(thrust_ratio_2, rpm_ratio_2_squared, rpm_ratio_2_squared * 0.15f);

        // Verify power scales with cube of RPM
        float power_ratio_1 = powers[1] / powers[0];
        float power_ratio_2 = powers[2] / powers[0];
        float rpm_ratio_1_cubed = std::pow(test_rpms[1] / test_rpms[0], 3);
        float rpm_ratio_2_cubed = std::pow(test_rpms[2] / test_rpms[0], 3);

        EXPECT_NEAR(power_ratio_1, rpm_ratio_1_cubed, rpm_ratio_1_cubed * 0.2f);
        EXPECT_NEAR(power_ratio_2, rpm_ratio_2_cubed, rpm_ratio_2_cubed * 0.2f);
    }
}

// Test blade flapping response at various forward speeds
TEST_F(ExtendedRotorTests, BladeFlappingSpeedTest) {
    const std::array<float, 5> test_velocities = {0.0f, 5.0f, 10.0f, 15.0f, 20.0f};

    for (int i = 0; i < 5; ++i) {  // Test 5 different configurations
        configureRandomRotor();
        m_rotorData.currentRPM = getRandomFloat(3000.0f, 9000.0f);
        auto conditions = getStandardConditions();

        std::vector<float> flapping_angles;
        std::vector<float> coning_angles;

        for (float velocity : test_velocities) {
            m_rotorBody->setLinearVelocity(btVector3(velocity, 0, 0));
            rotor::physics::update_blade_state(&m_rotorData, velocity, conditions, 0.016f);

            flapping_angles.push_back(m_rotorData.blade_state.flapping_angle);
            coning_angles.push_back(m_rotorData.blade_state.coning_angle);

            // Verify reasonable angles
            EXPECT_GE(m_rotorData.blade_state.flapping_angle, 0.0f);
            EXPECT_LE(m_rotorData.blade_state.flapping_angle, 0.25f);
            EXPECT_GE(m_rotorData.blade_state.coning_angle, 0.0f);
            EXPECT_LE(m_rotorData.blade_state.coning_angle, 0.2f);
        }

        // Verify flapping increases with forward speed
        for (size_t j = 1; j < test_velocities.size(); ++j) {
            EXPECT_GT(flapping_angles[j], flapping_angles[j-1]);
        }
    }
}

// Test turbulence response at different altitudes
// In ExtendedRotorTests.h
TEST_F(ExtendedRotorTests, AltitudeTurbulenceTest) {
    // Test at meteorologically significant altitudes
    const std::array<float, 5> test_altitudes = {
        6.1f,     // 20ft - reference height
        100.0f,   // Surface layer
        1000.0f,  // Boundary layer transition
        3000.0f,  // Free atmosphere
        5000.0f   // High altitude
    };
    const float test_airspeed = 10.0f;
    const float test_time = 1.0f;

    std::vector<models::TurbulenceState> turbulence_states;

    for (float altitude : test_altitudes) {
        auto conditions = models::calculate_atmospheric_conditions(altitude, test_airspeed);
        auto turb_state = models::calculate_turbulence(
            altitude,
            test_airspeed,
            conditions,
            test_time
        );
        turbulence_states.push_back(turb_state);

        // Basic sanity checks for each altitude
        EXPECT_GT(turb_state.velocity.length(), 0.0f);
        EXPECT_GT(turb_state.intensity, 0.0f);
        EXPECT_LT(turb_state.velocity.length(), 15.0f);
    }

    // Verify characteristics of boundary layer transition
    EXPECT_GT(turbulence_states[0].intensity, turbulence_states[2].intensity)
        << "Surface turbulence should generally be more intense than at boundary layer transition";

    // Verify free atmosphere characteristics
    EXPECT_GT(turbulence_states[2].length_scale, turbulence_states[0].length_scale)
        << "Turbulence length scales should increase with altitude";

    // Verify variation between altitudes exists
    bool has_variation = false;
    for (size_t i = 1; i < turbulence_states.size(); ++i) {
        if (std::abs(turbulence_states[i].velocity.length() -
                     turbulence_states[i-1].velocity.length()) > 0.01f) {
            has_variation = true;
            break;
        }
    }
    EXPECT_TRUE(has_variation) << "Turbulence should vary with altitude";

    // Check that length scales follow expected patterns
    for (size_t i = 1; i < turbulence_states.size(); ++i) {
        EXPECT_GE(turbulence_states[i].length_scale, turbulence_states[i-1].length_scale)
            << "Turbulence length scales should generally increase or stay constant with altitude";
    }
}
} // namespace lark::tests