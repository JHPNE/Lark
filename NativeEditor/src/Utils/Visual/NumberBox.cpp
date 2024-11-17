#include "NumberBox.h"
#include <algorithm>
#include <imgui_internal.h>

// Static member initialization
bool NumberBox::s_dragStarted = false;
float NumberBox::s_dragStartValue = 0.0f;
float NumberBox::s_dragStartMouseX = 0.0f;
ImGuiID NumberBox::s_editingId = 0;
ImGuiID NumberBox::s_activeId = 0;

bool NumberBox::Draw(const char* label, float* value, const Config& config) {
    bool value_changed = false;
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiStyle& style = ImGui::GetStyle();
    const float frame_height = ImGui::GetFrameHeight();
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

    float w = ImGui::CalcItemWidth();
    if (config.showButtons) {
        float button_size = (config.buttonSize > 0) ? config.buttonSize : frame_height;
        w -= (button_size + style.ItemInnerSpacing.x) * 2;
    }

    const ImGuiID id = window->GetID(label);
    const ImVec2 frame_min = window->DC.CursorPos;
    const ImVec2 frame_max(frame_min.x + w, frame_min.y + frame_height);
    const ImRect frame_bb(frame_min, frame_max);

    const ImVec2 total_max(
        frame_max.x + (label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f),
        frame_max.y
    );
    const ImRect total_bb(frame_min, total_max);

    ImGui::ItemSize(total_bb, style.FramePadding.y);
    if (!ImGui::ItemAdd(total_bb, id))
        return false;

    const bool hovered = ImGui::ItemHoverable(frame_bb, id, ImGuiItemFlags_None);

    if (hovered) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (!s_dragStarted) {
                s_dragStarted = true;
                s_dragStartValue = *value;
                s_dragStartMouseX = ImGui::GetIO().MousePos.x;
                s_activeId = id;
            }

            if (s_activeId == id) {
                float dragSpeed = config.dragSpeed;
                if (ImGui::GetIO().KeyCtrl)
                    dragSpeed *= 0.1f;
                else if (ImGui::GetIO().KeyShift)
                    dragSpeed *= 10.0f;

                float dragDelta = ImGui::GetIO().MousePos.x - s_dragStartMouseX;
                float newValue = s_dragStartValue + dragDelta * dragSpeed;
                newValue = std::clamp(newValue, config.min, config.max);

                if (newValue != *value) {
                    *value = newValue;
                    value_changed = true;
                }
            }
        }
        else {
            if (s_activeId == id) {
                s_dragStarted = false;
                s_activeId = 0;
            }
        }

        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            ImGui::SetKeyboardFocusHere();
            s_editingId = id;
        }
    }

    ImGui::RenderFrame(frame_bb.Min, frame_bb.Max,
        ImGui::GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

    char value_buf[64];
    if (s_editingId == id) {
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

        static char input_buf[64];
        snprintf(input_buf, IM_ARRAYSIZE(input_buf), "%.*f", config.decimals, *value);

        ImGui::SetNextItemWidth(w);
        if (ImGui::InputText("##edit", input_buf, IM_ARRAYSIZE(input_buf),
                           ImGuiInputTextFlags_CharsDecimal | ImGuiInputTextFlags_EnterReturnsTrue)) {
            *value = std::clamp((float)atof(input_buf), config.min, config.max);
            value_changed = true;
            s_editingId = 0;
        }
        else if (!ImGui::IsItemActive()) {
            s_editingId = 0;
        }

        ImGui::PopStyleVar();
    }
    else {
        snprintf(value_buf, IM_ARRAYSIZE(value_buf), "%.*f", config.decimals, *value);

        ImVec2 text_min(frame_bb.Min.x + style.FramePadding.x,
                       frame_bb.Min.y + style.FramePadding.y);
        ImVec2 text_max(frame_bb.Max.x - style.FramePadding.x,
                       frame_bb.Max.y - style.FramePadding.y);

        ImGui::RenderTextClipped(text_min, text_max, value_buf, NULL, NULL, ImVec2(0.5f, 0.5f));
    }

    if (config.showButtons) {
        float button_size = (config.buttonSize > 0) ? config.buttonSize : frame_height;

        ImGui::SameLine(0, style.ItemInnerSpacing.x);
        if (ImGui::Button("-", ImVec2(button_size, button_size))) {
            *value = std::clamp(*value - config.dragSpeed, config.min, config.max);
            value_changed = true;
        }

        ImGui::SameLine(0, style.ItemInnerSpacing.x);
        if (ImGui::Button("+", ImVec2(button_size, button_size))) {
            *value = std::clamp(*value + config.dragSpeed, config.min, config.max);
            value_changed = true;
        }
    }

    if (label_size.x > 0.0f) {
        ImGui::SameLine(0, style.ItemInnerSpacing.x);
        ImGui::TextUnformatted(label);
    }

    return value_changed;
}