#pragma once

#include "Utils/ErrorHandling.h"
#include "Utils/Logger.h"
#include <gtest/gtest.h>
#include <memory>
#include <sstream>
#include <string>

namespace utils
{
namespace test
{

/**
 * @brief Test fixture for error handling and logging validation
 *
 * Test Requirements:
 * - REQ-TEST-01: Must verify all error severity levels
 * - REQ-TEST-02: Must verify error code uniqueness
 * - REQ-TEST-03: Must verify location tracking accuracy
 * - REQ-TEST-04: Must verify message formatting
 * - REQ-TEST-05: Must verify thread safety
 *
 * Test Categories:
 * - CAT-01: Basic Functionality
 * - CAT-02: Error Handling
 * - CAT-03: Edge Cases
 * - CAT-04: Performance
 */
class LoggingTest : public ::testing::Test
{
  protected:
    /**
     * @brief Set up test environment
     *
     * Ensures logger is properly initialized before each test.
     */
    void SetUp() override
    {
        // Initialize logger with console output only
        ASSERT_TRUE(Logger::GetInstance().Initialize(Logger::Level::TRACE));
    }

    /**
     * @brief Tear down test environment
     *
     * Ensures cleanup after each test case.
     */
    void TearDown() override
    {
        // No cleanup needed for console-only logging
    }

    /**
     * @brief Verify message contains expected content
     *
     * @param expectedContent Content that should be present
     * @param message Full message to check
     * @return true if content is found
     */
    static bool ContainsContent(const std::string &expectedContent,
                                const std::string &message) noexcept
    {
        return message.find(expectedContent) != std::string::npos;
    }
};

/**
 * @brief CAT-01: Basic Error Validation Tests
 */
TEST_F(LoggingTest, ValidationErrorBasicTest)
{
    // Test parameters
    const std::string expectedMessage = "Invalid parameter value";

    // Exercise
    try
    {
        throw ValidationError(expectedMessage);
        FAIL() << "ValidationError not thrown as expected";
    }
    catch (const ValidationError &error)
    {
        // Verify error properties
        EXPECT_EQ(error.GetSeverity(), ErrorSeverity::MODERATE) << "Incorrect severity level";
        EXPECT_EQ(error.GetErrorCode(), ValidationError::ERROR_BASE) << "Incorrect error code";
        EXPECT_TRUE(ContainsContent(expectedMessage, error.what())) << "Error message mismatch";
    }
}

/**
 * @brief CAT-02: Custom Error Code Test
 */
TEST_F(LoggingTest, SimulationErrorWithCustomCodeTest)
{
    // Test parameters
    const std::uint32_t customCode = SimulationError::ERROR_BASE + 1;
    const std::string expectedMessage = "Simulation diverged";
    const ErrorSeverity expectedSeverity = ErrorSeverity::CRITICAL;

    // Exercise
    try
    {
        throw SimulationError(expectedMessage, customCode, expectedSeverity);
        FAIL() << "SimulationError not thrown as expected";
    }
    catch (const SimulationError &error)
    {
        // Verify error properties
        EXPECT_EQ(error.GetSeverity(), expectedSeverity) << "Severity level mismatch";
        EXPECT_EQ(error.GetErrorCode(), customCode) << "Custom error code mismatch";
        EXPECT_TRUE(ContainsContent(expectedMessage, error.what())) << "Error message mismatch";
    }
}

/**
 * @brief CAT-02: Numeric Error Validation
 */
TEST_F(LoggingTest, NumericErrorTest)
{
    // Test parameters
    const std::string expectedMessage = "Division by zero";

    // Exercise
    try
    {
        throw NumericError(expectedMessage);
        FAIL() << "NumericError not thrown as expected";
    }
    catch (const NumericError &error)
    {
        // Verify error properties
        EXPECT_EQ(error.GetSeverity(), ErrorSeverity::SEVERE) << "Incorrect severity level";
        EXPECT_EQ(error.GetErrorCode(), NumericError::ERROR_BASE) << "Incorrect error code";
        EXPECT_TRUE(ContainsContent(expectedMessage, error.what())) << "Error message mismatch";
    }
}

/**
 * @brief CAT-03: Range Validation Macro Test
 */
TEST_F(LoggingTest, ValidateRangeMacroTest)
{
    // Test parameters
    const float testValue = 5.0f;
    const float minValue = 0.0f;
    const float maxValue = 1.0f;
    const std::string expectedMessage = "Value out of range";

    // Exercise
    try
    {
        VALIDATE_RANGE(testValue, minValue, maxValue, expectedMessage);
        FAIL() << "ValidationError not thrown for out-of-range value";
    }
    catch (const ValidationError &error)
    {
        // Verify error message
        EXPECT_TRUE(ContainsContent(expectedMessage, error.what())) << "Error message mismatch";
        // Verify severity
        EXPECT_EQ(error.GetSeverity(), ErrorSeverity::MODERATE) << "Incorrect severity level";
    }
}

/**
 * @brief CAT-02: Simulation Assertion Test
 */
TEST_F(LoggingTest, AssertSimulationMacroTest)
{
    // Test parameters
    const bool condition = false;
    const std::string expectedMessage = "Simulation assertion failed";

    // Exercise
    try
    {
        ASSERT_SIMULATION(condition, expectedMessage);
        FAIL() << "SimulationError not thrown for failed assertion";
    }
    catch (const SimulationError &error)
    {
        // Verify error message
        EXPECT_TRUE(ContainsContent(expectedMessage, error.what())) << "Error message mismatch";
        // Verify severity
        EXPECT_EQ(error.GetSeverity(), ErrorSeverity::SEVERE) << "Incorrect severity level";
    }
}

/**
 * @brief CAT-01: Location Tracking Test
 */
TEST_F(LoggingTest, LocationTrackingTest)
{
    // Exercise
    try
    {
        // Explicitly specify the location to ensure macro expansion happens here
        throw ValidationError("Test error", ValidationError::ERROR_BASE, ErrorSeverity::MODERATE,
                              MAKE_SOURCE_LOCATION());
        FAIL() << "ValidationError not thrown as expected";
    }
    catch (const ValidationError &error)
    {
        const SourceLocation &location = error.GetLocation();
        std::string filename = std::string(location.file);
        size_t lastSlash = filename.find_last_of("/\\");
        if (lastSlash != std::string::npos)
        {
            filename = filename.substr(lastSlash + 1);
        }

        std::string functionName = std::string(location.function);

        EXPECT_EQ(filename, "LoggingTest.h") << "Incorrect source file";
        EXPECT_TRUE(functionName.find("LocationTrackingTest") != std::string::npos)
            << "Incorrect function name";
        EXPECT_GT(location.line, 0) << "Invalid line number";
    }
}

/**
 * @brief CAT-03: Error Code Range Validation
 */
TEST_F(LoggingTest, ErrorCodeRangeTest)
{
    // Verify error code ranges are properly separated
    EXPECT_LT(ValidationError::ERROR_BASE, SimulationError::ERROR_BASE)
        << "Validation and Simulation error codes overlap";
    EXPECT_LT(SimulationError::ERROR_BASE, NumericError::ERROR_BASE)
        << "Simulation and Numeric error codes overlap";

    // Verify minimum separation between error bases
    constexpr std::uint32_t MIN_SEPARATION = 1000;
    EXPECT_GE(SimulationError::ERROR_BASE - ValidationError::ERROR_BASE, MIN_SEPARATION)
        << "Insufficient separation between error code ranges";
    EXPECT_GE(NumericError::ERROR_BASE - SimulationError::ERROR_BASE, MIN_SEPARATION)
        << "Insufficient separation between error code ranges";
}

} // namespace test
} // namespace utils