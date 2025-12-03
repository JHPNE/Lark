#include "CustomWidgets.h"
#include <imgui_internal.h>

namespace LarkStyle {

float CustomWidgets::s_propertyLabelWidth = Sizing::PropertyLabelWidth;
int CustomWidgets::s_propertyDepth = 0;

void CustomWidgets::Initialize() {
    ApplyRefinedTheme();
}

bool CustomWidgets::BeginSection(const char* label, bool defaultOpen) {
    // Add spacing between sections
    if (s_propertyDepth == 0) {
        ImGui::Dummy(ImVec2(0, Sizing::SectionSpacing));
    }

    // Section header styling
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 8));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Header, Colors::SectionHeader);
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.20f, 0.20f, 0.21f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.22f, 0.22f, 0.23f, 1.0f));

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Framed |
                               ImGuiTreeNodeFlags_SpanAvailWidth |
                               ImGuiTreeNodeFlags_FramePadding |
                               ImGuiTreeNodeFlags_AllowOverlap;
    if (defaultOpen) flags |= ImGuiTreeNodeFlags_DefaultOpen;

    bool open = ImGui::CollapsingHeader(label, flags);

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);

    if (open) {
        // Only create child window if section is open
        ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, Sizing::Rounding);
        ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::SectionBg);

        // Create a unique ID for the child window
        char child_id[256];
        snprintf(child_id, sizeof(child_id), "%s_content", label);

        ImGuiChildFlags child_flags = ImGuiChildFlags_AlwaysAutoResize | ImGuiChildFlags_AutoResizeY;
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar;

        ImGui::BeginChild(child_id, ImVec2(0, 0), child_flags, window_flags);

        ImGui::Spacing();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
        ImGui::Indent(8.0f);
        s_propertyDepth++;
    }

    return open;
}

void CustomWidgets::EndSection() {
    if (s_propertyDepth > 0) {
        s_propertyDepth--;
        ImGui::Unindent(8.0f);
        ImGui::PopStyleVar(); // ItemSpacing
        ImGui::Spacing();

        ImGui::EndChild();
        ImGui::PopStyleColor(); // ChildBg
        ImGui::PopStyleVar(); // ChildRounding
    }
}

bool CustomWidgets::PropertyFloat(const char* label, float* value,
                                  float min, float max, const char* format) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(Colors::TextDim, "%s", label);
    ImGui::SameLine(s_propertyLabelWidth);

    ImGui::PushItemWidth(Sizing::PropertyControlWidth);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Sizing::Rounding);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, Colors::InputBg);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Colors::InputBgHover);
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Colors::InputBgActive);
    ImGui::PushStyleColor(ImGuiCol_Border, Colors::BorderSubtle);

    char id[256];
    snprintf(id, sizeof(id), "##%s", label);

    bool changed = false;
    if (min != max) {
        changed = ImGui::DragFloat(id, value, (max - min) * 0.005f, min, max, format);
    } else {
        changed = ImGui::InputFloat(id, value, 0.0f, 0.0f, format);
    }

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar();
    ImGui::PopItemWidth();

    return changed;
}

bool CustomWidgets::PropertyFloat3(const char* label, float* values, const char* format) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(Colors::TextDim, "%s", label);
    ImGui::SameLine(s_propertyLabelWidth);

    ImGui::PushItemWidth(Sizing::PropertyControlWidth);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Sizing::Rounding);

    char id[256];
    snprintf(id, sizeof(id), "##%s", label);

    // Push an ID to make each component unique
    ImGui::PushID(id);

    float itemWidth = (Sizing::PropertyControlWidth - 8.0f) / 3.0f;
    bool changed = false;

    const char* labels[] = {"##X", "##Y", "##Z"};
    for (int i = 0; i < 3; i++) {
        if (i > 0) ImGui::SameLine(0, 4);

        ImGui::PushItemWidth(itemWidth);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, Colors::InputBg);
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Colors::InputBgHover);
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, Colors::InputBgActive);
        ImGui::PushStyleColor(ImGuiCol_Border, Colors::BorderSubtle);

        // Use the predefined label to ensure uniqueness
        changed |= ImGui::DragFloat(labels[i], &values[i], 0.01f, 0.0f, 0.0f, format);

        ImGui::PopStyleColor(4);
        ImGui::PopItemWidth();
    }

    ImGui::PopID(); // Pop the property ID
    ImGui::PopStyleVar();
    ImGui::PopItemWidth();

    return changed;
}

bool CustomWidgets::PropertyBool(const char* label, bool* value) {
    ImGui::AlignTextToFramePadding();
    ImGui::TextColored(Colors::TextDim, "%s", label);
    ImGui::SameLine(s_propertyLabelWidth);

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);
    ImGui::PushStyleColor(ImGuiCol_FrameBg, Colors::InputBg);
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, Colors::InputBgHover);
    ImGui::PushStyleColor(ImGuiCol_CheckMark, Colors::Accent);

    char id[256];
    snprintf(id, sizeof(id), "##%s", label);
    bool changed = ImGui::Checkbox(id, value);

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    return changed;
}

bool CustomWidgets::PropertyToggle(const char* label, bool* value) {
    return PropertyBool(label, value);
}

bool CustomWidgets::Button(const char* label, const ImVec2& size) {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Sizing::Rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14, 8));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.26f, 0.26f, 0.27f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.30f, 0.31f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.22f, 0.22f, 0.23f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextBright);
    ImGui::PushStyleColor(ImGuiCol_Border, Colors::BorderSubtle);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);

    bool clicked = ImGui::Button(label, size);

    ImGui::PopStyleVar(3);
    ImGui::PopStyleColor(5);

    return clicked;
}

bool CustomWidgets::AccentButton(const char* label, const ImVec2& size) {
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Sizing::Rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(12, 6));
    ImGui::PushStyleColor(ImGuiCol_Button, Colors::Accent);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, Colors::AccentHover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, Colors::AccentActive);
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextBright);

    bool clicked = ImGui::Button(label, size);

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(2);

    return clicked;
}

bool CustomWidgets::ColoredButton(const char* label, WidgetColorType type, const ImVec2& size) {
    ImVec4 color, colorHover, colorActive;

    switch(type) {
    case WidgetColorType::Primary:
        color = ImVec4(0.35f, 0.35f, 0.40f, 1.0f);
        break;
    case WidgetColorType::Secondary:
        color = ImVec4(0.25f, 0.25f, 0.28f, 1.0f);
        break;
    case WidgetColorType::Info:
        color = Colors::AccentInfo;
        break;
    case WidgetColorType::Success:
        color = Colors::AccentSuccess;
        break;
    case WidgetColorType::Warning:
        color = Colors::AccentWarning;
        break;
    case WidgetColorType::Danger:
        color = Colors::AccentDanger;
        break;
    default:
        return Button(label, size);
    }

    colorHover = ImVec4(
        std::min(color.x * 1.15f, 1.0f),
        std::min(color.y * 1.15f, 1.0f),
        std::min(color.z * 1.15f, 1.0f),
        color.w
    );
    colorActive = ImVec4(
        color.x * 0.85f,
        color.y * 0.85f,
        color.z * 0.85f,
        color.w
    );

    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, Sizing::Rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(14, 8));
    ImGui::PushStyleColor(ImGuiCol_Button, color);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, colorHover);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, colorActive);
    ImGui::PushStyleColor(ImGuiCol_Text, Colors::TextBright);

    bool clicked = ImGui::Button(label, size);

    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(2);

    return clicked;
}

void CustomWidgets::BeginPropertyGrid(const char* id, float labelWidth) {
    ImGui::PushID(id);
    if (labelWidth > 0) {
        s_propertyLabelWidth = labelWidth;
    }
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 10));
}

void CustomWidgets::EndPropertyGrid() {
    ImGui::PopStyleVar();
    ImGui::PopID();
}

void CustomWidgets::BeginPropertyTable(const char* id) {
    BeginPropertyGrid(id);
}

void CustomWidgets::EndPropertyTable() {
    EndPropertyGrid();
}

bool CustomWidgets::BeginPanel(const char* label, const ImVec2& size) {
    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, Sizing::Rounding);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::BackgroundMid);
    ImGui::PushStyleColor(ImGuiCol_Border, Colors::Border);

    // Use proper flags for child windows
    ImGuiChildFlags child_flags = ImGuiChildFlags_Borders;
    return ImGui::BeginChild(label, size, child_flags, 0);
}

void CustomWidgets::EndPanel() {
    ImGui::EndChild();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

bool CustomWidgets::BeginToolbar(const char* id) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(4, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, Colors::BackgroundDark);

    // Use proper flags
    ImGuiChildFlags child_flags = ImGuiChildFlags_Borders;
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoScrollbar |
                                    ImGuiWindowFlags_NoScrollWithMouse;

    return ImGui::BeginChild(id, ImVec2(0, 32), child_flags, window_flags);
}

void CustomWidgets::EndToolbar() {
    ImGui::EndChild();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

bool CustomWidgets::ToolbarButton(const char* label, const char* tooltip) {
    bool clicked = ImGui::Button(label, ImVec2(0, 24));

    if (tooltip && ImGui::IsItemHovered()) {
        ImGui::BeginTooltip();
        ImGui::Text("%s", tooltip);
        ImGui::EndTooltip();
    }

    ImGui::SameLine();
    return clicked;
}

void CustomWidgets::ToolbarSeparator() {
    ImGui::SameLine();
    ImGui::Text("|");
    ImGui::SameLine();
}

void CustomWidgets::Separator(const char* label) {
    if (label) {
        ImGui::Spacing();
        ImGui::TextColored(Colors::TextDim, "%s", label);
    }

    ImGui::PushStyleColor(ImGuiCol_Separator, Colors::BorderSubtle);
    ImGui::Separator();
    ImGui::PopStyleColor();
    ImGui::Spacing();
}

void CustomWidgets::SeparatorText(const char* label) {
    Separator(label);
}

} // namespace LarkStyle