#pragma once
#include <imgui.h>
#include <string>
#include <functional>
#include <cmath>

class NumberBox {
public:
	NumberBox() = default;
	virtual ~NumberBox() = default;

	void Draw(const char* label, float* value, float multiplier = 1.0f);

protected:
	struct DragState {
		bool active = false;
		float originalValue = 0.0f;
		float mouseXStart = 0.0f;
		bool valueChanged = false;
		float multiplier = 0.01f;
	};

	bool HandleDrag(float* value, DragState& state, float multiplier);
	void HandleTextInput(float* value);

private:
	bool m_isEditing = false;
	char m_inputBuffer[32] = "";
	DragState m_dragState;
};

