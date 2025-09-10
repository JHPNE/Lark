#pragma once
#include <Eigen/Core>
#include <Eigen/Dense>
#include <Eigen/Geometry>

namespace lark::physics_math
{
// Type aliases for cleaner code
using Vector3f = Eigen::Vector3f;
using Vector4f = Eigen::Vector4f;
using Matrix3f = Eigen::Matrix3f;
using Matrix4f = Eigen::Matrix4f;
using Matrix4x3f = Eigen::Matrix<float, 4, 3>;
using Matrix3x4f = Eigen::Matrix<float, 3, 4>;
using Quaternionf = Eigen::Quaternionf;

inline float PI = 3.141592653589793238462643383280f;

// Utility functions
inline Matrix3f hatMap(const Vector3f &v)
{
    Matrix3f S;
    S << 0, -v.z(), v.y(), v.z(), 0, -v.x(), -v.y(), v.x(), 0;
    return S;
}

inline Vector3f veeMap(const Matrix3f &S) { return Vector3f(-S(2, 1), S(2, 0), -S(1, 0)); }

inline Matrix3f quaternionToRotationMatrix(const Vector4f &q)
{
    // q is [x, y, z, w] format
    Quaternionf quat(q(3), q(0), q(1), q(2)); // Eigen uses (w,x,y,z) order
    return quat.toRotationMatrix();
}

inline Vector4f rotationMatrixToQuaternion(const Matrix3f &R)
{
    Quaternionf q(R);
    return Vector4f(q.x(), q.y(), q.z(), q.w()); // Return as [x,y,z,w]
}

inline Vector4f quatDot(const Vector4f &quat, const Vector3f &omega)
{
    float q0 = quat(0); // x
    float q1 = quat(1); // y
    float q2 = quat(2); // z
    float q3 = quat(3); // w

    Matrix4x3f G_T;
    G_T << q3, -q2, q1, q2, q3, -q0, -q1, q0, q3, -q0, -q1, -q2;

    Vector4f quat_dot_vec = (0.5f * G_T * omega).eval();

    // Augment to maintain unit quaternion constraint
    float quat_err = quat.squaredNorm() - 1.0f;
    Vector4f quat_err_grad = 2.0f * quat;

    return (quat_dot_vec - quat_err * quat_err_grad).eval();
}
} // namespace lark::physics_math