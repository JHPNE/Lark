#pragma once
#include "imgui.h"

// Applies a modern dark style with transparency.
void ApplyModernDarkStyle();

// Optionally draw a gradient background inside the current ImGui window.
void DrawWindowGradientBackground(const ImVec4& topColor, const ImVec4& bottomColor);
