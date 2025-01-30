#include <gtest/gtest.h>
#include <cmath>
#include "DroneExtension/Components/Models/BladeFlapping.h"

namespace lark::tests {

class BladeFlappingTest : public ::testing::Test {
protected:
    // Standard atmospheric conditions at sea level
    const float kSeaLevelDensity = 1.225f;  // kg/m³
    
    // Common rotor parameters for UH-60 like helicopter
    const float kNominalRotorSpeed = 27.0f;  // rad/s (~258 RPM)
    const float kBladeMass = 110.0f;         // kg
    const float kLockNumber = 8.0f;          // typical value for medium helicopter
    // Increased blade_grip to 5.0m for realistic rotor size
    const float kBladeGrip = 5.0f;           // m (from hinge to tip)
    const float kHingeOffset = 0.381f;       // m
    const float kSpringConstant = 54250.0f;  // Increased for larger blade
    
    // Tolerance for floating-point comparisons
    const float kTolerance = 1e-4f;
    
    // Setup standard blade properties
    lark::models::BladeProperties standard_props;

    float cbi(const models::BladeProperties& blade_props) {
        return blade_props.mass * std::pow(blade_props.blade_grip, 2) / 3.0f;
    }
    
    void SetUp() override {
        standard_props.mass = kBladeMass;
        standard_props.hinge_offset = kHingeOffset;
        standard_props.lock_number = kLockNumber;
        standard_props.spring_constant = kSpringConstant;
        // Correct inertia calculation: I = mass * blade_grip² / 3
        float I_beta = (kBladeMass * std::pow(kBladeGrip, 2)) / 3.0f;
        standard_props.blade_grip = kBladeGrip;

        standard_props.natural_frequency = std::sqrt(
           standard_props.spring_constant / cbi(standard_props)
           );
    }
};

// Test hover conditions
TEST_F(BladeFlappingTest, HoverConditions) {
    float rotor_speed = kNominalRotorSpeed;
    float forward_velocity = 0.0f;
    float collective_pitch = 0.087f;  // ~5 degrees
    float cyclic_pitch = 0.0f;
    float shaft_tilt = 0.0f;
    float delta_time = 0.001f;
    
    lark::models::BladeState state = lark::models::calculate_blade_state(
        standard_props,
        rotor_speed,
        forward_velocity,
        kSeaLevelDensity,
        collective_pitch,
        cyclic_pitch,
        shaft_tilt,
        delta_time
    );
    
    // In hover, expect small coning angle (2-4 degrees typically)
    EXPECT_GT(state.coning_angle, 0.035f);  // > 2 degrees
    EXPECT_LT(state.coning_angle, 0.070f);  // < 4 degrees
    
    // In pure hover, expect near-zero flapping
    EXPECT_NEAR(state.flapping_angle, 0.0f, 0.01f);
    
    // TPP should be nearly horizontal in hover
    float expected_z = std::cos(state.coning_angle);
    EXPECT_NEAR(state.tip_path_plane.z(), expected_z, 0.001f);  // Relax tolerance to 0.00
    EXPECT_NEAR(state.tip_path_plane.y(), 0.0f, 0.001f);       // Allow slight tilt
}

// Test forward flight conditions
TEST_F(BladeFlappingTest, ForwardFlight) {
    float rotor_speed = kNominalRotorSpeed;
    float forward_velocity = 40.0f;  // ~80 knots
    float collective_pitch = 0.105f;  // ~6 degrees
    float cyclic_pitch = 0.052f;     // ~3 degrees
    float shaft_tilt = -0.087f;      // ~-5 degrees nose down
    float delta_time = 0.001f;
    
    lark::models::BladeState state = lark::models::calculate_blade_state(
        standard_props,
        rotor_speed,
        forward_velocity,
        kSeaLevelDensity,
        collective_pitch,
        cyclic_pitch,
        shaft_tilt,
        delta_time
    );
    
    // Expect increased coning in forward flight
    EXPECT_GT(state.coning_angle, 0.052f);  // > 3 degrees
    
    // Expect non-zero flapping in forward flight
    EXPECT_NE(state.flapping_angle, 0.0f);
    
    // TPP should tilt back in forward flight
    EXPECT_GT(state.tip_path_plane.x(), 0.0f);
}

// Test physical limits and constraints
TEST_F(BladeFlappingTest, PhysicalConstraints) {
    float rotor_speed = kNominalRotorSpeed;
    float forward_velocity = 20.0f;
    float collective_pitch = 0.175f;  // 10 degrees
    float cyclic_pitch = 0.087f;      // 5 degrees
    float shaft_tilt = 0.0f;
    float delta_time = 0.001f;
    
    lark::models::BladeState state = lark::models::calculate_blade_state(
        standard_props,
        rotor_speed,
        forward_velocity,
        kSeaLevelDensity,
        collective_pitch,
        cyclic_pitch,
        shaft_tilt,
        delta_time
    );
    
    // Flapping angle should never exceed physical limits (typically ±15 degrees)
    EXPECT_LT(std::abs(state.flapping_angle), 0.262f);  // 15 degrees
    
    // Lead-lag angle should be small
    EXPECT_LT(std::abs(state.lead_lag_angle), 0.087f);  // 5 degrees
    
    // Disk loading should be positive and within reasonable limits
    EXPECT_GT(state.disk_loading, 0.0f);
    EXPECT_LT(state.disk_loading, 500.0f);  // N/m²
}

// Test blade natural frequency
TEST_F(BladeFlappingTest, NaturalFrequency) {
    // For articulated rotors, natural frequency should be less than rotor speed
    EXPECT_LT(standard_props.natural_frequency, kNominalRotorSpeed);
    
    // But not too low (typically > 0.7 * rotor speed)
    EXPECT_GT(standard_props.natural_frequency, 0.7f * kNominalRotorSpeed);
}

// Test extreme conditions
TEST_F(BladeFlappingTest, ExtremeConditions) {
    float rotor_speed = kNominalRotorSpeed * 1.2f;  // Overspeed
    float forward_velocity = 80.0f;  // High speed
    float collective_pitch = 0.262f;  // 15 degrees
    float cyclic_pitch = 0.175f;      // 10 degrees
    float shaft_tilt = -0.175f;       // -10 degrees
    float delta_time = 0.001f;
    
    lark::models::BladeState state = lark::models::calculate_blade_state(
        standard_props,
        rotor_speed,
        forward_velocity,
        kSeaLevelDensity,
        collective_pitch,
        cyclic_pitch,
        shaft_tilt,
        delta_time
    );
    
    // Even in extreme conditions, angles should remain within physical limits
    EXPECT_LT(std::abs(state.flapping_angle), 0.524f);  // 30 degrees absolute max
    EXPECT_LT(std::abs(state.lead_lag_angle), 0.262f);  // 15 degrees max
    
    // Tip path plane normal vector should be unit length
    float tpp_magnitude = std::sqrt(
        std::pow(state.tip_path_plane.x(), 2) +
        std::pow(state.tip_path_plane.y(), 2) +
        std::pow(state.tip_path_plane.z(), 2)
    );
    EXPECT_NEAR(tpp_magnitude, 1.0f, kTolerance);
}

// Test convergence of flapping motion
TEST_F(BladeFlappingTest, Convergence) {
    float rotor_speed = kNominalRotorSpeed;
    float forward_velocity = 30.0f;
    float collective_pitch = 0.105f;
    float cyclic_pitch = 0.052f;
    float shaft_tilt = -0.052f;
    
    // Run multiple time steps
    lark::models::BladeState prev_state;
    lark::models::BladeState current_state;
    
    for (int i = 0; i < 1000; i++) {
        prev_state = current_state;
        current_state = lark::models::calculate_blade_state(
            standard_props,
            rotor_speed,
            forward_velocity,
            kSeaLevelDensity,
            collective_pitch,
            cyclic_pitch,
            shaft_tilt,
            0.001f
        );
        
        if (i > 900) {  // Check convergence in last 100 steps
            // Flapping rate should be small
            EXPECT_NEAR(current_state.flapping_rate, 0.0f, 0.1f);
            
            // State should be stable
            EXPECT_NEAR(current_state.flapping_angle, 
                       prev_state.flapping_angle, 0.01f);
        }
    }
}
}