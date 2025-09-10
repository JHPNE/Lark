#include "Style.h"

// Internal helper functions (not exposed)
static void SetModernDarkStyleVariables(ImGuiStyle &style);
static void SetModernDarkBaseColors(ImGuiStyle &style);

void ApplyModernDarkStyle()
{
    ImGuiStyle &style = ImGui::GetStyle();

    // Setup layout, rounding, padding, etc.
    SetModernDarkStyleVariables(style);

    // Setup base colors with some transparency
    SetModernDarkBaseColors(style);

    // If you want to load custom fonts, do so here:
    // ImGuiIO& io = ImGui::GetIO();
    // io.Fonts->AddFontFromFileTTF("path/to/your/font.ttf", 16.0f);
    // ImGui::RebuildFonts();
}

void DrawWindowGradientBackground(const ImVec4 &topColor, const ImVec4 &bottomColor)
{
    auto ToU32 = [](const ImVec4 &c) { return ImGui::ColorConvertFloat4ToU32(c); };

    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetWindowPos();
    ImVec2 size = ImGui::GetWindowSize();

    // Draw a vertical gradient from topColor to bottomColor
    draw_list->AddRectFilledMultiColor(pos, ImVec2(pos.x + size.x, pos.y + size.y), ToU32(topColor),
                                       ToU32(topColor), ToU32(bottomColor), ToU32(bottomColor));
}

static void SetModernDarkStyleVariables(ImGuiStyle &style)
{
    style.WindowPadding = ImVec2(10.0f, 10.0f);
    style.FramePadding = ImVec2(6.0f, 4.0f);
    style.ItemSpacing = ImVec2(8.0f, 8.0f);
    style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);

    style.WindowRounding = 6.0f;
    style.ChildRounding = 4.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;
    style.ScrollbarRounding = 3.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 4.0f;

    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    style.PopupBorderSize = 1.0f;

    style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
    style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
    style.SelectableTextAlign = ImVec2(0.0f, 0.5f);

    style.ScrollbarSize = 14.0f;
    style.GrabMinSize = 10.0f;
}

static void SetModernDarkBaseColors(ImGuiStyle &style)
{
    ImVec4 *colors = style.Colors;

    // Text
    colors[ImGuiCol_Text] = ImVec4(0.90f, 0.92f, 0.95f, 1.00f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.51f, 0.52f, 1.00f);

    // Backgrounds (slight transparency)
    colors[ImGuiCol_WindowBg] = ImVec4(0.10f, 0.11f, 0.12f, 0.95f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.11f, 0.12f, 0.00f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.10f, 0.95f);

    // Borders
    colors[ImGuiCol_Border] = ImVec4(0.20f, 0.20f, 0.23f, 0.50f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);

    // Frame BG
    colors[ImGuiCol_FrameBg] = ImVec4(0.17f, 0.18f, 0.20f, 0.90f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.20f, 0.21f, 0.23f, 0.90f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.22f, 0.23f, 0.25f, 0.90f);

    // Title
    colors[ImGuiCol_TitleBg] = ImVec4(0.08f, 0.08f, 0.09f, 1.00f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.10f, 0.10f, 0.11f, 1.00f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.05f, 0.05f, 0.05f, 0.75f);

    // Menus
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.11f, 0.12f, 0.13f, 0.95f);

    // Scrollbar
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.40f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.35f, 0.90f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.35f, 0.35f, 0.40f, 0.90f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.40f, 0.40f, 0.45f, 0.90f);

    // CheckMark
    colors[ImGuiCol_CheckMark] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);

    // Sliders
    colors[ImGuiCol_SliderGrab] = ImVec4(0.28f, 0.60f, 0.99f, 0.90f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.36f, 0.69f, 1.00f, 0.90f);

    // Buttons
    colors[ImGuiCol_Button] = ImVec4(0.20f, 0.20f, 0.25f, 0.90f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.30f, 0.95f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);

    // Headers
    colors[ImGuiCol_Header] = ImVec4(0.20f, 0.20f, 0.25f, 0.90f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.25f, 0.25f, 0.30f, 0.95f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.30f, 0.30f, 0.35f, 1.00f);

    // Separator
    colors[ImGuiCol_Separator] = ImVec4(0.25f, 0.24f, 0.24f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.35f, 0.34f, 0.34f, 1.00f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.45f, 0.44f, 0.44f, 1.00f);

    // Resize Grips
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.29f, 0.30f, 0.33f, 0.70f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.35f, 0.36f, 0.39f, 0.80f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.40f, 0.41f, 0.44f, 0.90f);

    // Tabs
    colors[ImGuiCol_Tab] = ImVec4(0.18f, 0.19f, 0.20f, 0.90f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.25f, 0.26f, 0.29f, 0.95f);
    colors[ImGuiCol_TabActive] = ImVec4(0.22f, 0.23f, 0.25f, 1.00f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.15f, 0.16f, 0.17f, 0.90f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.18f, 0.19f, 0.20f, 0.90f);

    // Plot
    colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.74f, 0.74f, 0.74f, 1.00f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.60f, 0.68f, 0.25f, 1.00f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.65f, 0.75f, 0.30f, 1.00f);

    // Text Selection
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);

    // Drag Drop
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.26f, 0.59f, 0.98f, 0.95f);

    // Navigation Highlight
    colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 0.80f);

    // Modal Window
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.10f, 0.10f, 0.10f, 0.50f);
}
