#pragma once

#include <cstdint>

namespace utils
{

/**
 * @brief Location tracking data structure for error reporting and logging
 *
 * Provides standardized source code location tracking following
 * DoD-STD-2167A section 4.2.4 for diagnostic data requirements.
 */
struct SourceLocation
{
    /** @brief Source file path (static lifetime) */
    const char *const file;

    /** @brief Line number in source file */
    const std::uint32_t line;

    /** @brief Function name (static lifetime) */
    const char *const function;

    /**
     * @brief Construct location information
     *
     * @param f Source file name (must have static lifetime)
     * @param l Source line number
     * @param fn Function name (must have static lifetime)
     */
    constexpr SourceLocation(const char *f, const std::uint32_t l, const char *fn) noexcept
        : file(f ? f : "unknown"), line(l), function(fn ? fn : "unknown")
    {
    }

    /**
     * @brief Default location constructor
     *
     * @return Location marked as unknown
     */
    static constexpr SourceLocation Unknown() noexcept
    {
        return SourceLocation("unknown", 0, "unknown");
    }
};

// Platform-specific function name detection
#if defined(_MSC_VER) // Microsoft Visual C++
#define CURRENT_FUNCTION __FUNCSIG__
#elif defined(__GNUC__) // GCC/Clang
#define CURRENT_FUNCTION __PRETTY_FUNCTION__
#else
#define CURRENT_FUNCTION __func__
#endif

/**
 * @brief Create source location for current position
 *
 * Creates standardized location information using compiler intrinsics.
 */
#define MAKE_SOURCE_LOCATION()                                                                     \
    ::utils::SourceLocation(__FILE__, static_cast<std::uint32_t>(__LINE__), CURRENT_FUNCTION)

} // namespace utils