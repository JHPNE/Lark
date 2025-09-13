// MathUtils.h
#pragma once
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace MathUtils
{
constexpr float Epsilon = 0.00001f;

inline bool IsEqual(float a, float b) { return std::abs(a - b) < Epsilon; }

inline bool IsEqual(const glm::vec3 &a, const glm::vec3 &b)
{
    return IsEqual(a.x, b.x) && IsEqual(a.y, b.y) && IsEqual(a.z, b.z);
}
} // namespace MathUtils