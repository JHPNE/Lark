#pragma once
#include <string>

class Utils {
public:
    static void SetEnvironmentVariable(const std::string& name, const std::string& value);
    static std::string GetEnvironmentVariable(const std::string& name);
    static bool ShowSetEnginePathPopup();

    static bool s_showEnginePathPopup;
};