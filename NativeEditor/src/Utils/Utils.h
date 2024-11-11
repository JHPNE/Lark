// Utils.h
#pragma once
#include <string>
#include <filesystem>
#include <optional>

namespace fs = std::filesystem;

class Utils {
public:
    static void SetEnvironmentVariable(const std::string& name, const std::string& value);
    static std::string GetEnvironmentVariable(const std::string& name);
    static bool ShowSetEnginePathPopup();
    static bool s_showEnginePathPopup;

    // New methods for platform-specific path handling
    static fs::path GetApplicationDataPath();
    static fs::path GetEngineResourcePath();
    static std::optional<fs::path> GetBundlePath();
    static fs::path GetDefaultEnginePath();
};