#pragma once
#include <gtest/gtest.h>
#include <cmath>
#include "DroneExtension/Components/Models/ISA.h"

namespace lark::tests {
class ISAModelTest : public ::testing::Test {
protected:
    // Constants for ISA model at sea level
    const float kSeaLevelTemperature = 288.15f;  // K
    const float kSeaLevelPressure = 101325.0f;   // Pa
    const float kSeaLevelDensity = 1.225f;       // kg/m³
    const float kSeaLevelViscosity = 1.789e-5f;  // kg/(m·s)
    const float kSeaLevelSpeedOfSound = 340.294f; // m/s

    // Adjusted tolerances for different parameters
    static constexpr float kTemperatureTolerance = 0.01f;    // 0.01K accuracy
    static constexpr float kPressureTolerance = 0.5f;        // 0.5Pa accuracy
    static constexpr float kDensityTolerance = 0.001f;       // 0.001kg/m³ accuracy
    static constexpr float kViscosityTolerance = 1e-7f;      // Viscosity tolerance
    static constexpr float kSpeedOfSoundTolerance = 0.01f;   // 0.01m/s accuracy

    void ExpectNearWithTolerance(float actual, float expected, float tolerance, const char* param_name) {
        EXPECT_NEAR(actual, expected, tolerance)
            << param_name << " deviation exceeds tolerance. "
            << "Actual: " << actual << ", Expected: " << expected;
    }

    void ValidateAtmosphericConditions(
        const models::AtmosphericConditions& conditions,
        float expected_temp,
        float expected_pressure,
        float expected_density,
        const char* test_desc) {

        ExpectNearWithTolerance(conditions.temperature, expected_temp,
                               kTemperatureTolerance, "Temperature");
        ExpectNearWithTolerance(conditions.pressure, expected_pressure,
                               kPressureTolerance, "Pressure");
        ExpectNearWithTolerance(conditions.density, expected_density,
                               kDensityTolerance, "Density");

        // Verify speed of sound calculation
        float expected_speed_of_sound =
            std::sqrt(models::ISA_GAMMA * models::ISA_GAS_CONSTANT * conditions.temperature);
        ExpectNearWithTolerance(conditions.speed_of_sound, expected_speed_of_sound,
                               kSpeedOfSoundTolerance, "Speed of Sound");

        // Verify ideal gas law
        float calculated_density = conditions.pressure /
            (models::ISA_GAS_CONSTANT * conditions.temperature);
        ExpectNearWithTolerance(conditions.density, calculated_density,
                               kDensityTolerance, "Gas Law Density");
    }
};

TEST_F(ISAModelTest, SeaLevelConditions) {
    models::AtmosphericConditions conditions =
        models::calculate_atmospheric_conditions(0.0f, 0.0f);

    ValidateAtmosphericConditions(conditions,
                                 kSeaLevelTemperature,
                                 kSeaLevelPressure,
                                 kSeaLevelDensity,
                                 "Sea Level");
}

TEST_F(ISAModelTest, SpecificAltitudes) {
    struct TestCase {
        float altitude;
        float expected_temp;
        float expected_pressure;
        float expected_density;
    };

    std::vector<TestCase> test_cases = {
        {2000.0f, 275.15f, 79495.2f, 1.0065f},
        {5000.0f, 255.65f, 54019.9f, 0.7364f},
        {8000.0f, 236.15f, 35600.1f, 0.5258f}
    };

    for (const auto& test : test_cases) {
        models::AtmosphericConditions conditions =
            models::calculate_atmospheric_conditions(test.altitude, 0.0f);

        ValidateAtmosphericConditions(conditions,
                                    test.expected_temp,
                                    test.expected_pressure,
                                    test.expected_density,
                                    "Specific Altitude");
    }
}

TEST_F(ISAModelTest, NegativeAltitude) {
    EXPECT_THROW(models::calculate_atmospheric_conditions(-100.0f, 0.0f),
                 std::invalid_argument);
}

TEST_F(ISAModelTest, ExtremeAltitude) {
    EXPECT_THROW(models::calculate_atmospheric_conditions(90000.0f, 0.0f),
                 std::out_of_range);
}

TEST_F(ISAModelTest, TroposphereLimit) {
    models::AtmosphericConditions conditions =
        models::calculate_atmospheric_conditions(11000.0f, 0.0f);

    ValidateAtmosphericConditions(conditions,
                                 216.65f,   // Temperature at tropopause
                                 22632.1f,  // Pressure at tropopause
                                 0.364f,    // Density at tropopause
                                 "Tropopause");
}

} // namespace lark::tests