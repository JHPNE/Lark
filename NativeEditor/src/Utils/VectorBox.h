#pragma once
#include "NumberBox.h"
#include <array>

class VectorBox {
public:
    static bool Draw(const char* label, float* values, int components, const NumberBox::Config& config = NumberBox::Config());

private:
    static const char* s_labels[];
    static const ImVec4 s_colors[];
};