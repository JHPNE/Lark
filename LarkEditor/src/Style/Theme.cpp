#include "Theme.h"

namespace LarkStyle {

void ApplyRefinedTheme() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Window
    colors[ImGuiCol_WindowBg] = Colors::BackgroundDarkest;
    colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_PopupBg] = Colors::BackgroundPanel;
    colors[ImGuiCol_Border] = Colors::Border;
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.3f);

    // Frame
    colors[ImGuiCol_FrameBg] = Colors::InputBg;
    colors[ImGuiCol_FrameBgHovered] = Colors::InputBgHover;
    colors[ImGuiCol_FrameBgActive] = Colors::InputBgActive;

    // Title
    colors[ImGuiCol_TitleBg] = Colors::BackgroundDark;
    colors[ImGuiCol_TitleBgActive] = Colors::BackgroundMid;
    colors[ImGuiCol_TitleBgCollapsed] = Colors::BackgroundDark;

    // Menu
    colors[ImGuiCol_MenuBarBg] = Colors::BackgroundDark;

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.54f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.34f, 0.34f, 0.34f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.40f, 0.40f, 0.40f, 0.54f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.56f, 0.56f, 0.56f, 0.54f);

    // Check Mark
    colors[ImGuiCol_CheckMark] = Colors::AccentInfo;

    // Slider
    colors[ImGuiCol_SliderGrab] = Colors::Accent;
    colors[ImGuiCol_SliderGrabActive] = Colors::AccentActive;

    // Button - more subtle
    colors[ImGuiCol_Button] = Colors::BackgroundLight;
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.28f, 0.28f, 0.29f, 1.00f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.20f, 0.20f, 0.21f, 1.00f);

    // Headers
    colors[ImGuiCol_Header] = Colors::SectionHeader;
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.20f, 0.21f, 1.00f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.25f, 0.25f, 0.26f, 1.00f);

    // Separator
    colors[ImGuiCol_Separator] = Colors::BorderSubtle;
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);

    // Tab
    colors[ImGuiCol_Tab] = Colors::BackgroundMid;
    colors[ImGuiCol_TabHovered] = Colors::BackgroundLight;
    colors[ImGuiCol_TabActive] = Colors::BackgroundLight;
    colors[ImGuiCol_TabUnfocused] = Colors::BackgroundDark;
    colors[ImGuiCol_TabUnfocusedActive] = Colors::BackgroundMid;

    // Docking
    colors[ImGuiCol_DockingPreview] = Colors::AccentInfo;
    colors[ImGuiCol_DockingEmptyBg] = Colors::DockingBg;

    // Text
    colors[ImGuiCol_Text] = Colors::Text;
    colors[ImGuiCol_TextDisabled] = Colors::TextDisabled;
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);

    // Table
    colors[ImGuiCol_TableHeaderBg] = Colors::SectionHeader;
    colors[ImGuiCol_TableBorderStrong] = Colors::Border;
    colors[ImGuiCol_TableBorderLight] = Colors::BorderSubtle;

    // Style
    style.WindowPadding = ImVec2(Sizing::WindowPadding, Sizing::WindowPadding);
    style.FramePadding = ImVec2(Sizing::FramePadding, 5.0f);
    style.ItemSpacing = ImVec2(Sizing::ItemSpacing, Sizing::ItemSpacing);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.IndentSpacing = Sizing::IndentSpacing;
    style.ScrollbarSize = Sizing::ScrollbarSize;
    style.GrabMinSize = 12.0f;

    // Rounding
    style.WindowRounding = Sizing::WindowRounding;
    style.ChildRounding = Sizing::Rounding;
    style.FrameRounding = Sizing::Rounding;
    style.PopupRounding = Sizing::Rounding;
    style.ScrollbarRounding = Sizing::Rounding;
    style.GrabRounding = Sizing::GrabRounding;
    style.TabRounding = Sizing::TabRounding;

    // Borders
    style.WindowBorderSize = 1.0f;
    style.ChildBorderSize = 1.0f;
    style.PopupBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.TabBorderSize = 0.0f;

    // Alignment
    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.5f);

    // Docking specific
    style.TabCloseButtonMinWidthSelected = 0.0f;
    style.DockingSeparatorSize = 2.0f;
}

} // namespace LarkStyle