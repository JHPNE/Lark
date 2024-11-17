#pragma once
#include <imgui.h>

class NumberBox {
public:
    struct Config {
        float dragSpeed = 0.01f;
        float min = -100.0f;
        float max = 100.0f;
        int decimals;
        bool showButtons;
        float buttonSize;

        Config() :
            dragSpeed(0.01f),
            min(-FLT_MAX),
            max(FLT_MAX),
            decimals(3),
            showButtons(false),
            buttonSize(0.0f)
        {}
    };

    static bool Draw(const char* label, float* value, const Config& config = Config());

private:
    static bool s_dragStarted;
    static float s_dragStartValue;
    static float s_dragStartMouseX;
    static ImGuiID s_editingId;
    static ImGuiID s_activeId;
};