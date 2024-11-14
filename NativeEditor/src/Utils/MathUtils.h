// MathUtils.h
#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

namespace MathUtils {
    constexpr float Epsilon = 0.00001f;

    inline bool IsEqual(float a, float b) {
        return std::abs(a - b) < Epsilon;
    }

    inline bool IsEqual(const glm::vec3& a, const glm::vec3& b) {
        return IsEqual(a.x, b.x) && IsEqual(a.y, b.y) && IsEqual(a.z, b.z);
    }

    struct Vec3 {
        float x, y, z;

        Vec3() : x(0.0f), y(0.0f), z(0.0f) {}
        Vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
        explicit Vec3(float v) : x(v), y(v), z(v) {}

        // Basic operators
        Vec3 operator+(const Vec3& rhs) const { return Vec3(x + rhs.x, y + rhs.y, z + rhs.z); }
        Vec3 operator-(const Vec3& rhs) const { return Vec3(x - rhs.x, y - rhs.y, z - rhs.z); }
        Vec3 operator*(float scalar) const { return Vec3(x * scalar, y * scalar, z * scalar); }
        Vec3 operator/(float scalar) const {
            float inv = 1.0f / scalar;
            return Vec3(x * inv, y * inv, z * inv);
        }

        // Assignment operators
        Vec3& operator+=(const Vec3& rhs) {
            x += rhs.x; y += rhs.y; z += rhs.z;
            return *this;
        }

        Vec3& operator-=(const Vec3& rhs) {
            x -= rhs.x; y -= rhs.y; z -= rhs.z;
            return *this;
        }

        Vec3& operator*=(float scalar) {
            x *= scalar; y *= scalar; z *= scalar;
            return *this;
        }

        Vec3& operator/=(float scalar) {
            float inv = 1.0f / scalar;
            x *= inv; y *= inv; z *= inv;
            return *this;
        }

        // Utility functions
        float Length() const { return std::sqrt(x * x + y * y + z * z); }
        float LengthSquared() const { return x * x + y * y + z * z; }

        void Normalize() {
            float len = Length();
            if (len > 0) {
                float inv = 1.0f / len;
                x *= inv; y *= inv; z *= inv;
            }
        }

        Vec3 Normalized() const {
            Vec3 result = *this;
            result.Normalize();
            return result;
        }

        static Vec3 Zero() { return Vec3(0.0f, 0.0f, 0.0f); }
        static Vec3 One() { return Vec3(1.0f, 1.0f, 1.0f); }
    };
}