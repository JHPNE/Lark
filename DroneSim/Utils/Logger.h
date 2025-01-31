#pragma once

#include "SourceLocation.h"
#include <string>
#include <mutex>
#include <iostream>
#include <sstream>
#include <array>
#include <chrono>
#include <ctime>

namespace utils {

/**
 * @brief Thread-safe console logging system with DoD-compliant severity levels
 *
 * System Requirements:
 * - REQ-LOG-01: Thread-safe logging operations
 * - REQ-LOG-02: Precise timestamp tracking
 * - REQ-LOG-03: Location tracking
 * - REQ-LOG-04: Severity filtering
 * - REQ-LOG-05: Guaranteed message delivery
 *
 * Design Constraints:
 * - CON-LOG-01: Must operate without file system dependencies
 * - CON-LOG-02: Must maintain thread safety
 * - CON-LOG-03: Must never throw exceptions
 */
class Logger {
public:
    /**
     * @brief Severity levels for log messages
     *
     * Compliant with MIL-STD-1629A severity classifications
     */
    enum class Level : uint8_t {
        TRACE   = 0,  ///< Detailed tracing information
        DEBUG   = 1,  ///< Debugging information
        INFO    = 2,  ///< General information
        WARNING = 3,  ///< Warning conditions
        ERROR   = 4,  ///< Error conditions
        FATAL   = 5   ///< Critical failures
    };

    /**
     * @brief Get singleton instance of logger
     *
     * @return Reference to logger instance
     * @throws None
     */
    static Logger& GetInstance() noexcept {
        static Logger instance;
        return instance;
    }

    /**
     * @brief Initialize the logging system
     *
     * @param minLevel Minimum severity level to log
     * @return true Always succeeds for console logging
     * @throws None
     */
    bool Initialize(Level minLevel = Level::INFO) noexcept {
        std::lock_guard<std::mutex> lock(mutex_);
        minLevel_ = minLevel;
        initialized_ = true;
        // Remove logging during initialization to prevent recursive locks
        return true;
    }

    /**
     * @brief Write log entry to console with thread safety
     *
     * @param level Message severity level
     * @param message Log message content
     * @param location Source code location
     * @throws None
     */
    void WriteLogEntryToConsole(
        Level level,
        const std::string& message,
        const SourceLocation& location = SourceLocation::Unknown()
    ) noexcept {
        if (!initialized_ || level < minLevel_) {
            return;
        }

        std::string formattedMessage;
        try {
            // Format message outside the lock
            formattedMessage = FormatLogMessage(level, message, location);
        } catch (...) {
            return;
        }

        try {
            std::lock_guard<std::mutex> lock(mutex_);
            // Use cerr for ERROR and FATAL, cout for others
            if (level >= Level::ERROR) {
                std::cerr << formattedMessage << std::flush;
            } else {
                std::cout << formattedMessage << std::flush;
            }
        } catch (...) {
            // Ensure logging never throws
        }
    }

    // Convenience methods
    void Trace(const std::string& message,
               const SourceLocation& location = MAKE_SOURCE_LOCATION()) noexcept {
        WriteLogEntryToConsole(Level::TRACE, message, location);
    }

    void Debug(const std::string& message,
               const SourceLocation& location = MAKE_SOURCE_LOCATION()) noexcept {
        WriteLogEntryToConsole(Level::DEBUG, message, location);
    }

    void Info(const std::string& message,
              const SourceLocation& location = MAKE_SOURCE_LOCATION()) noexcept {
        WriteLogEntryToConsole(Level::INFO, message, location);
    }

    void Warning(const std::string& message,
                const SourceLocation& location = MAKE_SOURCE_LOCATION()) noexcept {
        WriteLogEntryToConsole(Level::WARNING, message, location);
    }

    void Error(const std::string& message,
               const SourceLocation& location = MAKE_SOURCE_LOCATION()) noexcept {
        WriteLogEntryToConsole(Level::ERROR, message, location);
    }

    void Fatal(const std::string& message,
               const SourceLocation& location = MAKE_SOURCE_LOCATION()) noexcept {
        WriteLogEntryToConsole(Level::FATAL, message, location);
    }

private:
    // Prevent external construction and copying
    Logger() = default;
    ~Logger() = default;
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    /**
     * @brief Convert severity level to string representation
     *
     * @param level Severity level to convert
     * @return String representation of level
     * @throws None
     */
    static const char* LevelToString(Level level) noexcept {
        static constexpr std::array<const char*, 6> LevelStrings = {
            "TRACE", "DEBUG", "INFO", "WARNING", "ERROR", "FATAL"
        };
        return LevelStrings[static_cast<size_t>(level)];
    }

    /**
     * @brief Format timestamp for log entry
     *
     * @param time Time point to format
     * @return Formatted timestamp string
     * @throws None
     */
    static std::string FormatTimestamp(
        const std::chrono::system_clock::time_point& time
    ) noexcept {
        try {
            const auto timer = std::chrono::system_clock::to_time_t(time);
            char buffer[32];

            #ifdef _WIN32
                struct tm timeinfo;
                localtime_s(&timeinfo, &timer);
                strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
            #else
                struct tm* timeinfo = localtime(&timer);
                strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeinfo);
            #endif

            return std::string(buffer);
        } catch (...) {
            return "TIME_ERROR";
        }
    }

    std::string FormatLogMessage(
        Level level,
        const std::string& message,
        const SourceLocation& location
    ) const noexcept {
        try {
            std::stringstream ss;
            ss << FormatTimestamp(std::chrono::system_clock::now())
               << " [" << LevelToString(level) << "] "
               << "[" << location.file << ":" << location.line << "] "
               << "[" << location.function << "] "
               << message << std::endl;
            return ss.str();
        } catch (...) {
            return message + "\n";
        }
    }

    std::mutex mutex_;                  ///< Thread synchronization mutex
    Level minLevel_{Level::INFO};       ///< Minimum logging level
    bool initialized_{false};           ///< Initialization flag
};

} // namespace utils

// Global logging macros
#define LOG_TRACE(msg) ::utils::Logger::GetInstance().Trace((msg), MAKE_SOURCE_LOCATION())
#define LOG_DEBUG(msg) ::utils::Logger::GetInstance().Debug((msg), MAKE_SOURCE_LOCATION())
#define LOG_INFO(msg) ::utils::Logger::GetInstance().Info((msg), MAKE_SOURCE_LOCATION())
#define LOG_WARNING(msg) ::utils::Logger::GetInstance().Warning((msg), MAKE_SOURCE_LOCATION())
#define LOG_ERROR(msg) ::utils::Logger::GetInstance().Error((msg), MAKE_SOURCE_LOCATION())
#define LOG_FATAL(msg) ::utils::Logger::GetInstance().Fatal((msg), MAKE_SOURCE_LOCATION())