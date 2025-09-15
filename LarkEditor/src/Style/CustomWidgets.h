#pragma once
#include "Theme.h"
#include <functional>
#include <string>

namespace LarkStyle {

class CustomWidgets {
public:
    static void Initialize();

    // Sections
    static bool BeginSection(const char* label, bool defaultOpen = true);
    static void EndSection();

    // Properties
    static bool PropertyFloat(const char* label, float* value,
                              float min = 0.0f, float max = 0.0f,
                              const char* format = "%.2f");
    static bool PropertyFloat3(const char* label, float* values,
                               const char* format = "%.2f");
    static bool PropertyInt(const char* label, int* value,
                           int min = 0, int max = 0);
    static bool PropertyBool(const char* label, bool* value);
    static bool PropertyToggle(const char* label, bool* value);

    // Buttons
    static bool Button(const char* label, const ImVec2& size = ImVec2(0, 0));
    static bool AccentButton(const char* label, const ImVec2& size = ImVec2(0, 0));
    static bool ColoredButton(const char* label, WidgetColorType type, const ImVec2& size = ImVec2(0, 0));

    // Layout
    static void BeginPropertyGrid(const char* id = "##PropertyGrid", float labelWidth = 0);
    static void EndPropertyGrid();
    static void BeginPropertyTable(const char* id = "##PropertyTable");
    static void EndPropertyTable();

    // Panels
    static bool BeginPanel(const char* label, const ImVec2& size = ImVec2(0, 0));
    static void EndPanel();

    // Toolbar
    static bool BeginToolbar(const char* id);
    static void EndToolbar();
    static bool ToolbarButton(const char* label, const char* tooltip = nullptr);
    static void ToolbarSeparator();

    // Separators
    static void Separator(const char* label = nullptr);
    static void SeparatorText(const char* label);

private:
    static float s_propertyLabelWidth;
    static int s_propertyDepth;
};

} // namespace LarkStyle