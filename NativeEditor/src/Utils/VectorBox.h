#pragma once
#include "NumberBox.h"
#include <array>
#include <cmath>

enum class VectorType {
	Vector2,
	Vector3,
	Vector4
};

class VectorBox {
public:
	VectorBox() = default;
	~VectorBox() = default;

	void Draw(const char* label, float* values, int components, float multiplier = 1.0f);
	void Draw(const char* label, VectorType type, float* values, float multiplier = 1.0f);
private:
	std::array<NumberBox, 4> m_numberBoxes;
	const char* const m_labels[4] = { "X", "Y", "Z", "W" };
};