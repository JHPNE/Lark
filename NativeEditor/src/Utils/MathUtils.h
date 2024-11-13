// MathUtils.h
#pragma once
#include <DirectXMath.h>

namespace MathUtils {
    constexpr float Epsilon = 0.00001f;

    inline bool IsEqual(float a, float b) {
        return std::abs(a - b) < Epsilon;
    }

    inline bool IsEqual(const DirectX::XMFLOAT3& a, const DirectX::XMFLOAT3& b) {
        return IsEqual(a.x, b.x) && IsEqual(a.y, b.y) && IsEqual(a.z, b.z);
    }
}