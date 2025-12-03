#include "CustomWindow.h"
#include <imgui_internal.h>

namespace LarkStyle {

bool CustomWindow::s_dockingEnabled = true;
std::unordered_map<std::string, CustomWindow::WindowState> CustomWindow::s_windowStates;
std::string CustomWindow::s_currentWindowName;

bool CustomWindow::Begin(const char* name, WindowConfig& config) {
    s_currentWindowName = name;

    // Get or create window state
    auto& windowState = s_windowStates[name];

    // Check if we need to undock THIS specific window
    if (windowState.startUndocking) {
        ImGui::SetNextWindowDockID(0, ImGuiCond_Always);
        ImGui::SetNextWindowPos(windowState.undockPosition, ImGuiCond_Always);
        windowState.startUndocking = false;
    }

    // Window flags
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoCollapse |
                                    ImGuiWindowFlags_NoTitleBar;

    if (!config.allowDocking) {
        window_flags |= ImGuiWindowFlags_NoDocking;
    }

    ImGui::SetNextWindowSizeConstraints(config.minSize, ImVec2(FLT_MAX, FLT_MAX));

    ImGuiWindow* existing_window = ImGui::FindWindowByName(name);
    bool is_docked = existing_window && existing_window->DockNode != nullptr;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, is_docked ? 1.0f : 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, is_docked ? 0.0f : Sizing::WindowRounding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, Colors::BackgroundDarkest);

    bool is_open = true;
    bool* p_open = config.p_open ? config.p_open : &is_open;

    if (ImGui::Begin(name, p_open, window_flags)) {
        windowState.inWindow = true;

        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->DockNode) {
            window->DockNode->LocalFlags |= ImGuiDockNodeFlags_NoTabBar;
        }

        DrawWindowHeader(config);
        HandleHeaderInteraction(name, config);

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 8));
        ImGui::BeginChild("ContentRegion", ImVec2(0, 0), false,
                         ImGuiWindowFlags_NoBackground |
                         ImGuiWindowFlags_NoScrollbar);
        ImGui::PopStyleVar();

        return true;
    }

    windowState.inWindow = false;
    return false;
}

void CustomWindow::End() {
    auto it = s_windowStates.find(s_currentWindowName);
    if (it != s_windowStates.end() && it->second.inWindow) {
        ImGui::EndChild();
        it->second.inWindow = false;
    }

    ImGui::End();
    ImGui::PopStyleColor(1);
    ImGui::PopStyleVar(3);
}

void CustomWindow::DrawWindowHeader(const WindowConfig& config) {
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 window_pos = ImGui::GetWindowPos();
    ImVec2 window_size = ImGui::GetWindowSize();

    // Header background
    float header_height = 36.0f;
    ImVec2 header_min = window_pos;
    ImVec2 header_max = ImVec2(window_pos.x + window_size.x, window_pos.y + header_height);

    // Check if window is docked
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    bool is_docked = window && window->DockNode != nullptr;

    draw_list->AddRectFilled(
        header_min,
        header_max,
        ImGui::GetColorU32(Colors::BackgroundDark),
        is_docked ? 0.0f : Sizing::WindowRounding,
        ImDrawFlags_RoundCornersTop
    );

    // Header separator line
    draw_list->AddLine(
        ImVec2(window_pos.x, window_pos.y + header_height),
        ImVec2(window_pos.x + window_size.x, window_pos.y + header_height),
        ImGui::GetColorU32(Colors::BorderSubtle),
        1.0f
    );

    // Visual feedback when shift is held over header (only for docked windows)
    ImVec2 mouse_pos = ImGui::GetMousePos();
    bool mouse_in_header = mouse_pos.x >= header_min.x && mouse_pos.x <= header_max.x &&
                           mouse_pos.y >= header_min.y && mouse_pos.y <= header_max.y;

    if (is_docked && mouse_in_header && ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
        // Draw highlight overlay
        draw_list->AddRectFilled(
            header_min,
            header_max,
            ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 0.05f)),
            Sizing::WindowRounding,
            ImDrawFlags_RoundCornersTop
        );

        // Change cursor to indicate draggable
        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);

        // Draw hint text
        const char* hint = "Shift+Drag to undock";
        ImVec2 text_size = ImGui::CalcTextSize(hint);
        ImVec2 hint_pos = ImVec2(header_max.x - text_size.x - 60, header_min.y + 11);
        draw_list->AddText(hint_pos, ImGui::GetColorU32(Colors::TextDim), hint);
    }

    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::BeginGroup();

    // Optional icon
    float text_offset = 12.0f;
    if (!config.icon.empty()) {
        ImGui::SetCursorPos(ImVec2(12, 10));
        ImGui::TextColored(Colors::TextDim, "%s", config.icon.c_str());
        text_offset = 36.0f;
    }

    // Window title
    ImGui::SetCursorPos(ImVec2(text_offset, 10));
    ImGui::TextColored(Colors::Text, "%s", config.title.c_str());

    // Close button (if p_open is provided)
    if (config.p_open) {
        float close_btn_size = 20.0f;
        ImGui::SetCursorPos(ImVec2(window_size.x - close_btn_size - 8, 8));

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Colors::AccentDanger);
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.2f, 0.2f, 1.0f));

        if (ImGui::Button("×", ImVec2(close_btn_size, close_btn_size))) {
            *config.p_open = false;
        }

        ImGui::PopStyleColor(3);
        ImGui::PopStyleVar(2);
    }

    ImGui::EndGroup();

    // Move cursor below header
    ImGui::SetCursorPos(ImVec2(0, header_height));
}

void CustomWindow::HandleHeaderInteraction(const char* window_id, const WindowConfig& config) {
    if (!config.allowDocking) return;

    ImGuiWindow* window = ImGui::GetCurrentWindow();
    bool is_docked = window && window->DockNode != nullptr;

    if (!is_docked) return;

    ImVec2 window_pos = ImGui::GetWindowPos();
    ImVec2 window_size = ImGui::GetWindowSize();
    float header_height = 36.0f;

    ImVec2 mouse_pos = ImGui::GetMousePos();
    ImVec2 header_min = window_pos;
    ImVec2 header_max = ImVec2(window_pos.x + window_size.x, window_pos.y + header_height);

    bool mouse_in_header = mouse_pos.x >= header_min.x && mouse_pos.x <= header_max.x &&
                           mouse_pos.y >= header_min.y && mouse_pos.y <= header_max.y;

    // Store undocking request for THIS specific window
    if (mouse_in_header && ImGui::IsMouseClicked(0) && ImGui::IsKeyDown(ImGuiKey_LeftShift)) {
        auto& windowState = s_windowStates[window_id];
        windowState.startUndocking = true;
        windowState.undockPosition = ImVec2(mouse_pos.x - window_size.x * 0.5f, mouse_pos.y - 10);
    }
}

} // namespace LarkStyle