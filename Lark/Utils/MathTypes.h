#pragma once

#include "../Common/CommonHeaders.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace lark::math
{
constexpr float pi = 3.1415926535897932384626433832795f;
constexpr float epsilon = 1e-5f;

// Vector types
using v2 = glm::vec2;
using v2a = glm::vec2; // GLM handles alignment internally
using v3 = glm::vec3;
using v3a = glm::vec3; // GLM handles alignment internally
using v4 = glm::vec4;
using v4a = glm::vec4; // GLM handles alignment internally

// Integer vector types
using u32v2 = glm::uvec2;
using u32v3 = glm::uvec3;
using u32v4 = glm::uvec4;
using s32v2 = glm::ivec2;
using s32v3 = glm::ivec3;
using s32v4 = glm::ivec4;

// Matrix types
using m3x3 = glm::mat3x3;
using m4x4 = glm::mat4x4;
using m4x4a = glm::mat4x4; // GLM handles alignment internally

template <u32 bits> constexpr u32 pack_unit_float(f32 f)
{
    static_assert(bits <= sizeof(u32) * 8);
    assert(f >= 0.f && f <= 1.f);
    constexpr f32 intervals{(f32)((1u << bits) - 1)}; // Changed 1ui32 to 1u
    return (u32)(intervals * f + 0.5f);
}

template <u32 bits> constexpr f32 unpack_unit_float(u32 i)
{
    static_assert(bits <= sizeof(u32) * 8);
    assert(i < (1u << bits)); // Changed 1ui32 to 1u
    constexpr f32 intervals{(f32)((1u << bits) - 1)};
    return (f32)i / intervals;
}

template <u32 bits> constexpr u32 pack_float(f32 f, f32 min, f32 max)
{
    assert(min < max);
    assert(f <= max && f >= min);
    const f32 distance{(f - min) / (max - min)};
    return pack_unit_float<bits>(distance);
}

template <u32 bits> constexpr f32 unpack_float(u32 i, f32 min, f32 max)
{
    assert(min < max);
    return unpack_unit_float<bits>(i) * (max - min) + min;
}

inline glm::quat quat_dot(const glm::quat &quat, const v3 &omega)
{
    // Note: GLM quat format is (w, x, y, z), input appears to be [i,j,k,w] = [x,y,z,w]
    float q0 = quat.x; // i
    float q1 = quat.y; // j
    float q2 = quat.z; // k
    float q3 = quat.w; // w

    // Original G was 3x4, so G.T is 4x3
    glm::mat4x3 G_T(
        // Column 1    Column 2    Column 3
        q3, -q2, q1,  // Row 1 -> q0 component
        q2, q3, -q0,  // Row 2 -> q1 component
        -q1, q0, q3,  // Row 3 -> q2 component
        -q0, -q1, -q2 // Row 4 -> q3 component
    );

    // Calculate quat_dot = 0.5 * G.T @ omega
    glm::vec4 quat_dot_vec = omega * (0.5f * G_T);

    // Augment to maintain unit quaternion constraint
    glm::vec4 quat_as_vec(q0, q1, q2, q3);
    float quat_err = glm::dot(quat_as_vec, quat_as_vec) - 1.0f;
    glm::vec4 quat_err_grad = 2.0f * quat_as_vec;

    auto test = quat_dot_vec - quat_err * quat_err_grad;
    return {test.w, test.x, test.y, test.z};
}

inline v3 normalize(const math::v3 &x)
{
    float length = sqrt(x.x * x.x + x.y * x.y + x.z * x.z);
    assert(length != 0);
    return x / length; // Divide each component by the length
}

inline v3 vee_map(const math::m3x3 &S)
{
    // Extract the vector from the skew-symmetric matrix
    float x = -S[1][2]; // From position (1,2): -(-x) = x
    float y = S[0][2];  // From position (0,2): y
    float z = -S[0][1]; // From position (0,1): -(z) = -z
    return v3(x, y, z);
}

inline m3x3 hatMap(const v3 &v) { return {0, -v.z, v.y, v.z, 0, -v.x, -v.y, v.x, 0}; }

inline m3x3 quaternionToRotationMatrix(const v4 &q)
{
    // Assuming q is [x, y, z, w] format
    glm::quat quaternion(q.w, q.x, q.y, q.z); // GLM uses (w,x,y,z) order
    return mat3_cast(quaternion);
}

inline v4 rotationMatrixToQuaternion(const m3x3 &R)
{
    glm::quat q = quat_cast(R);
    // GLM returns quaternion as (w,x,y,z), but if you need (x,y,z,w):
    return v4(q.x, q.y, q.z, q.w);
}
} // namespace lark::math