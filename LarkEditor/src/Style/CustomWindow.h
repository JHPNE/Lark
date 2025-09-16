#pragma once
#include "Theme.h"
#include <string>
#include <functional>

namespace LarkStyle {

class CustomWindow {
public:
    struct WindowConfig {
        std::string title;
        std::string icon = ""; // Optional icon, empty by default
        bool* p_open = nullptr;
        bool allowDocking = true;
        ImVec2 defaultSize = ImVec2(400, 600);
        ImVec2 minSize = ImVec2(200, 100);
        bool showToolbarActions = false;
        std::function<void()> customHeaderContent = nullptr;
    };

    static bool Begin(const char* name, WindowConfig& config);
    static void End();

    // Global docking state
    static bool IsDockingEnabled() { return s_dockingEnabled; }
    static void SetDockingEnabled(bool enabled) { s_dockingEnabled = enabled; }

private:
    struct WindowState {
        bool startUndocking = false;
        ImVec2 undockPosition;
        bool inWindow = false;
    };


    static bool s_dockingEnabled;
    static std::unordered_map<std::string, WindowState> s_windowStates;  // Per-window state
    static std::string s_currentWindowName;

    static void DrawWindowHeader(const WindowConfig& config);
    static void HandleHeaderInteraction(const char* window_id, const WindowConfig& config);
};

} // namespace LarkStyle