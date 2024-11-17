#include "VectorBox.h"
#include <imgui_internal.h>

const char* VectorBox::s_labels[] = { "X", "Y", "Z", "W" };
const ImVec4 VectorBox::s_colors[] = {
    ImVec4(0.9f, 0.1f, 0.1f, 0.7f), // Red for X
    ImVec4(0.1f, 0.9f, 0.1f, 0.7f), // Green for Y
    ImVec4(0.1f, 0.1f, 0.9f, 0.7f), // Blue for Z
    ImVec4(0.9f, 0.9f, 0.9f, 0.7f)  // White for W
};

bool VectorBox::Draw(const char* label, float* values, int components, const NumberBox::Config& config) {
    bool value_changed = false;
    ImGui::BeginGroup();

    const ImGuiStyle& style = ImGui::GetStyle();
    const float total_width = ImGui::CalcItemWidth();
    const float component_width = (total_width - style.ItemInnerSpacing.x * (components - 1)) / components;

    for (int i = 0; i < components; i++) {
        if (i > 0) ImGui::SameLine(0, style.ItemInnerSpacing.x);

        // Push component color
        ImGui::PushStyleColor(ImGuiCol_FrameBg,
            ImVec4(s_colors[i].x * 0.2f, s_colors[i].y * 0.2f, s_colors[i].z * 0.2f, s_colors[i].w));
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered,
            ImVec4(s_colors[i].x * 0.3f, s_colors[i].y * 0.3f, s_colors[i].z * 0.3f, s_colors[i].w));

        ImGui::PushID(i);
        ImGui::SetNextItemWidth(component_width);

        // Just use the ID for the label, making it invisible
        char component_label[32];
        snprintf(component_label, sizeof(component_label), "##%s%d", label, i);

        if (NumberBox::Draw(component_label, &values[i], config)) {
            value_changed = true;
        }

        ImGui::PopID();
        ImGui::PopStyleColor(2);
    }

    ImGui::EndGroup();
    return value_changed;
}