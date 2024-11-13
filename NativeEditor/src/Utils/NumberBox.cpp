#include "NumberBox.h"
#include <algorithm>
#include <cstring>

void NumberBox::Draw(const char* label, float* value, float multiplier) {
    ImGui::PushID(label);

    // Display current value
    ImGui::Text("%s: %.3f", label, *value);

    // Handle clicking and dragging
    if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        if (!m_dragState.active) {
            // Start drag operation
            m_dragState.active = true;
            m_dragState.originalValue = *value;
            m_dragState.mouseXStart = ImGui::GetIO().MousePos.x;
            m_dragState.valueChanged = false;
        }
    }

    if (m_dragState.active) {
        if (HandleDrag(value, m_dragState, multiplier)) {
            // Value changed through dragging
            m_isEditing = false;
        }

        // Handle drag release
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            m_dragState.active = false;
            if (!m_dragState.valueChanged) {
                // Click without drag - start text input
                m_isEditing = true;
                snprintf(m_inputBuffer, sizeof(m_inputBuffer), "%.3f", *value);
            }
        }
    }

    // Handle text input mode
    if (m_isEditing) {
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
        if (ImGui::InputText("##input", m_inputBuffer, sizeof(m_inputBuffer),
            ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
            HandleTextInput(value);
            m_isEditing = false;
        }

        // Exit edit mode on focus loss or escape
        if (!ImGui::IsItemActive() || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
            m_isEditing = false;
        }
    }

    ImGui::PopID();
}

bool NumberBox::HandleDrag(float* value, DragState& state, float multiplier) {
    float mouseDelta = ImGui::GetIO().MousePos.x - state.mouseXStart;

    if (std::abs(mouseDelta) > ImGui::GetIO().MouseDragThreshold) {
        // Adjust multiplier based on modifier keys
        float dragMultiplier = state.multiplier;
        if (ImGui::GetIO().KeyCtrl) dragMultiplier *= 0.1f;
        if (ImGui::GetIO().KeyShift) dragMultiplier *= 10.0f;

        *value = state.originalValue + (mouseDelta * dragMultiplier * multiplier);
        state.valueChanged = true;
        return true;
    }
    return false;
}

void NumberBox::HandleTextInput(float* value) {
    char* endPtr;
    float newValue = std::strtof(m_inputBuffer, &endPtr);
    if (endPtr != m_inputBuffer) {
        *value = newValue;
    }
}