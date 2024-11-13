#include "VectorBox.h"
#include "algorithm"

void VectorBox::Draw(const char* label, float* values, int components, float multiplier) {
    ImGui::BeginGroup();

    if (label && label[0] != '\0') {
        ImGui::Text("%s", label);
    }

    components = std::clamp(components, 2, 4);
    float spacing = ImGui::GetStyle().ItemSpacing.x;
    float width = (ImGui::GetContentRegionAvail().x - (spacing * (components - 1))) / components;

    for (int i = 0; i < components; ++i) {
        if (i > 0) ImGui::SameLine();
        ImGui::SetNextItemWidth(width);
        m_numberBoxes[i].Draw(m_labels[i], &values[i], multiplier);
    }

    ImGui::EndGroup();
}

void VectorBox::Draw(const char* label, VectorType type, float* values, float multiplier) {
    int components;
    switch (type) {
    case VectorType::Vector2: components = 2; break;
    case VectorType::Vector3: components = 3; break;
    case VectorType::Vector4: components = 4; break;
    default: components = 3; break;
    }
    Draw(label, values, components, multiplier);
}