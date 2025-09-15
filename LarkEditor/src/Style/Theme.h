#pragma once
#include <imgui.h>
#include <glm/glm.hpp>

namespace LarkStyle {

// More refined color palette - lighter and more sophisticated
namespace Colors {
    // Background hierarchy - lighter overall
    constexpr ImVec4 BackgroundDarkest = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    constexpr ImVec4 BackgroundDark = ImVec4(0.15f, 0.15f, 0.16f, 1.00f);
    constexpr ImVec4 BackgroundMid = ImVec4(0.18f, 0.18f, 0.19f, 1.00f);
    constexpr ImVec4 BackgroundLight = ImVec4(0.22f, 0.22f, 0.23f, 1.00f);
    constexpr ImVec4 BackgroundPanel = ImVec4(0.16f, 0.16f, 0.17f, 1.00f);

    // Section/container backgrounds
    constexpr ImVec4 SectionBg = ImVec4(0.14f, 0.14f, 0.15f, 1.00f);
    constexpr ImVec4 SectionHeader = ImVec4(0.17f, 0.17f, 0.18f, 1.00f);

    // More subtle accent colors
    constexpr ImVec4 Accent = ImVec4(0.45f, 0.45f, 0.50f, 1.00f);
    constexpr ImVec4 AccentHover = ImVec4(0.50f, 0.50f, 0.55f, 1.00f);
    constexpr ImVec4 AccentActive = ImVec4(0.40f, 0.40f, 0.45f, 1.00f);

    // Muted semantic colors
    constexpr ImVec4 AccentInfo = ImVec4(0.35f, 0.45f, 0.60f, 1.00f);
    constexpr ImVec4 AccentSuccess = ImVec4(0.35f, 0.55f, 0.35f, 1.00f);
    constexpr ImVec4 AccentWarning = ImVec4(0.65f, 0.55f, 0.30f, 1.00f);
    constexpr ImVec4 AccentDanger = ImVec4(0.60f, 0.35f, 0.35f, 1.00f);

    // Text hierarchy
    constexpr ImVec4 TextBright = ImVec4(0.92f, 0.92f, 0.92f, 1.00f);
    constexpr ImVec4 Text = ImVec4(0.80f, 0.80f, 0.80f, 1.00f);
    constexpr ImVec4 TextDim = ImVec4(0.60f, 0.60f, 0.60f, 1.00f);
    constexpr ImVec4 TextDisabled = ImVec4(0.40f, 0.40f, 0.40f, 1.00f);

    // Input fields - lighter
    constexpr ImVec4 InputBg = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    constexpr ImVec4 InputBgHover = ImVec4(0.12f, 0.12f, 0.13f, 1.00f);
    constexpr ImVec4 InputBgActive = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);

    // Borders
    constexpr ImVec4 BorderSubtle = ImVec4(0.28f, 0.28f, 0.29f, 0.40f);
    constexpr ImVec4 Border = ImVec4(0.32f, 0.32f, 0.33f, 0.60f);

    // Docking specific
    constexpr ImVec4 DockingBg = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    constexpr ImVec4 DockingActive = ImVec4(0.25f, 0.25f, 0.27f, 1.00f);
}

namespace Sizing {
    constexpr float WindowPadding = 12.0f;
    constexpr float FramePadding = 8.0f;
    constexpr float ItemSpacing = 10.0f;
    constexpr float IndentSpacing = 20.0f;
    constexpr float ScrollbarSize = 12.0f;
    constexpr float ControlHeight = 32.0f;
    constexpr float PropertyLabelWidth = 120.0f;
    constexpr float PropertyControlWidth = 200.0f;
    constexpr float Rounding = 4.0f;
    constexpr float WindowRounding = 6.0f;
    constexpr float GrabRounding = 3.0f;
    constexpr float TabRounding = 4.0f;
    constexpr float SectionSpacing = 2.0f;
}

enum class WidgetColorType {
    Default,
    Info,
    Success,
    Warning,
    Danger,
    Primary,
    Secondary
};

void ApplyRefinedTheme();

} // namespace LarkStyle