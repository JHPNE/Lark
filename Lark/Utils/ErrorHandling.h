#pragma once

#include "Logger.h"
#include "SourceLocation.h"
#include <cstdint>
#include <exception>
#include <string>

namespace utils
{

/**
 * @brief Error severity levels following MIL-STD-1629A
 *
 * System Requirements:
 * - REQ-ERR-01: Must provide standardized error classification
 * - REQ-ERR-02: Must support error severity tracking
 * - REQ-ERR-03: Must integrate with logging system
 * - REQ-ERR-04: Must provide error code management
 * - REQ-ERR-05: Must support source location tracking
 *
 * Design Constraints:
 * - CON-ERR-01: Must be thread-safe
 * - CON-ERR-02: Must be exception-safe
 * - CON-ERR-03: Must not allocate memory after construction
 */
enum class ErrorSeverity : std::uint8_t
{
    CRITICAL = 0, ///< Category I  - Catastrophic failure requiring immediate termination
    SEVERE = 1,   ///< Category II - Critical failure preventing normal operation
    MODERATE = 2, ///< Category III - Degraded operation but system can continue
    MINOR = 3,    ///< Category IV - Minor issue not affecting core functionality
    WARNING = 4   ///< Category V  - Potential issue requiring monitoring
};

/**
 * @brief Base class for all physics engine errors
 *
 * Implements MISRA C++:2008 compliant error handling with:
 * - Guaranteed thread safety through const correctness
 * - Zero-overhead exception specifications
 * - Comprehensive error tracking
 * - Automatic console logging
 * - Stack-based error information
 */
class ErrorHandling : public std::runtime_error
{
  public:
    /**
     * @brief Construct a new error with full context
     *
     * @param code Unique error identifier
     * @param message Detailed error description
     * @param severity Error severity level
     * @param location Source location information
     * @throws std::bad_alloc if memory allocation fails
     */
    explicit ErrorHandling(const std::uint32_t code, const std::string &message,
                           const ErrorSeverity severity,
                           const SourceLocation &location) noexcept(false)
        : std::runtime_error(FormatError(code, message)), code_(code), severity_(severity),
          location_(location)
    {
        LogErrorToConsole();
    }

    // Prevent slicing
    virtual ~ErrorHandling() = default;

    /**
     * @brief Get the error code
     * @return Unique error identifier
     * @throws None
     */
    std::uint32_t GetErrorCode() const noexcept { return code_; }

    /**
     * @brief Get the error severity
     * @return Error severity level
     * @throws None
     */
    ErrorSeverity GetSeverity() const noexcept { return severity_; }

    /**
     * @brief Get the error location
     * @return Source location information
     * @throws None
     */
    const SourceLocation &GetLocation() const noexcept { return location_; }

  protected:
    /**
     * @brief Format error message with code and details
     *
     * @param code Error code
     * @param message Error message
     * @return Formatted error string
     * @throws std::bad_alloc if memory allocation fails
     */
    static std::string FormatError(const std::uint32_t code, const std::string &message)
    {
        return "Error " + std::to_string(code) + ": " + message;
    }

    /**
     * @brief Log error to console with appropriate severity
     * @throws None
     */
    void LogErrorToConsole() const noexcept
    {
        try
        {
            std::string message = FormatError(code_, what());
            std::string locationInfo = std::string(location_.file) + ":" +
                                       std::to_string(location_.line) + " in " +
                                       std::string(location_.function);

            switch (severity_)
            {
            case ErrorSeverity::CRITICAL:
            case ErrorSeverity::SEVERE:
                std::cerr << "FATAL: " << message << " [at " << locationInfo << "]" << std::endl;
                break;
            case ErrorSeverity::MODERATE:
                std::cerr << "ERROR: " << message << " [at " << locationInfo << "]" << std::endl;
                break;
            case ErrorSeverity::MINOR:
            case ErrorSeverity::WARNING:
                std::cout << "WARNING: " << message << " [at " << locationInfo << "]" << std::endl;
                break;
            }
        }
        catch (...)
        {
            // Ensure logging never throws
        }
    }

  private:
    const std::uint32_t code_;      ///< Unique error identifier
    const ErrorSeverity severity_;  ///< Error severity level
    const SourceLocation location_; ///< Error source location
};

/**
 * @brief Validation error for parameter and state validation failures
 */
class ValidationError final : public ErrorHandling
{
  public:
    static constexpr std::uint32_t ERROR_BASE = 1000;

    explicit ValidationError(
        const std::string &message, const std::uint32_t code = ERROR_BASE,
        const ErrorSeverity severity = ErrorSeverity::MODERATE,
        const SourceLocation &location = MAKE_SOURCE_LOCATION()) noexcept(false)
        : ErrorHandling(code, message, severity, location)
    {
    }
};

/**
 * @brief Simulation error for physics engine runtime failures
 */
class SimulationError final : public ErrorHandling
{
  public:
    static constexpr std::uint32_t ERROR_BASE = 2000;

    explicit SimulationError(
        const std::string &message, const std::uint32_t code = ERROR_BASE,
        const ErrorSeverity severity = ErrorSeverity::SEVERE,
        const SourceLocation &location = MAKE_SOURCE_LOCATION()) noexcept(false)
        : ErrorHandling(code, message, severity, location)
    {
    }
};

/**
 * @brief Numeric error for mathematical computation failures
 */
class NumericError final : public ErrorHandling
{
  public:
    static constexpr std::uint32_t ERROR_BASE = 3000;

    explicit NumericError(const std::string &message, const std::uint32_t code = ERROR_BASE,
                          const ErrorSeverity severity = ErrorSeverity::SEVERE,
                          const SourceLocation &location = MAKE_SOURCE_LOCATION()) noexcept(false)
        : ErrorHandling(code, message, severity, location)
    {
    }
};

/**
 * @brief Safety-critical validation macro with location tracking
 */
#define VALIDATE(condition, message)                                                               \
    do                                                                                             \
    {                                                                                              \
        if (!(condition))                                                                          \
        {                                                                                          \
            throw ValidationError((message), ValidationError::ERROR_BASE, ErrorSeverity::MODERATE, \
                                  MAKE_SOURCE_LOCATION());                                         \
        }                                                                                          \
    } while (0)

/**
 * @brief Range validation macro with location tracking
 */
#define VALIDATE_RANGE(value, min, max, message)                                                   \
    do                                                                                             \
    {                                                                                              \
        if ((value) < (min) || (value) > (max))                                                    \
        {                                                                                          \
            throw ValidationError((message), ValidationError::ERROR_BASE, ErrorSeverity::MODERATE, \
                                  MAKE_SOURCE_LOCATION());                                         \
        }                                                                                          \
    } while (0)

/**
 * @brief Simulation assertion macro with location tracking
 */
#define ASSERT_SIMULATION(condition, message)                                                      \
    do                                                                                             \
    {                                                                                              \
        if (!(condition))                                                                          \
        {                                                                                          \
            throw SimulationError((message), SimulationError::ERROR_BASE, ErrorSeverity::SEVERE,   \
                                  MAKE_SOURCE_LOCATION());                                         \
        }                                                                                          \
    } while (0)

/**
 * @brief Numeric computation assertion macro with location tracking
 */
#define ASSERT_NUMERIC(condition, message)                                                         \
    do                                                                                             \
    {                                                                                              \
        if (!(condition))                                                                          \
        {                                                                                          \
            throw NumericError((message), NumericError::ERROR_BASE, ErrorSeverity::SEVERE,         \
                               MAKE_SOURCE_LOCATION());                                            \
        }                                                                                          \
    } while (0)

} // namespace utils