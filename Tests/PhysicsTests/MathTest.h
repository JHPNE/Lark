#include <gtest/gtest.h>
#include "Utils/MathTypes.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace lark::drones::test {
    using namespace math;

    class MathTest : public ::testing::Test {
    protected:
        // Helper function to check if two quaternions are approximately equal
        bool quat_near(const glm::quat& a, const glm::quat& b, float tolerance = 1e-5f) {
            return std::abs(a.w - b.w) < tolerance &&
                   std::abs(a.x - b.x) < tolerance &&
                   std::abs(a.y - b.y) < tolerance &&
                   std::abs(a.z - b.z) < tolerance;
        }
    };

    TEST_F(MathTest, QuatDot_ZeroAngularVelocity) {
        // Test with zero angular velocity - derivative should be zero (after constraint correction)
        glm::quat quat(1.0f, 0.0f, 0.0f, 0.0f);  // Unit quaternion (w=1, x=0, y=0, z=0)
        v3 omega(0.0f, 0.0f, 0.0f);               // Zero angular velocity

        glm::quat result = quat_dot(quat, omega);

        // Should be approximately zero
        EXPECT_NEAR(result.w, 0.0f, 1e-5f);
        EXPECT_NEAR(result.x, 0.0f, 1e-5f);
        EXPECT_NEAR(result.y, 0.0f, 1e-5f);
        EXPECT_NEAR(result.z, 0.0f, 1e-5f);
    }

    TEST_F(MathTest, QuatDot_SimpleRotation) {
        // Test with unit quaternion and simple angular velocity
        glm::quat quat(1.0f, 0.0f, 0.0f, 0.0f);  // Identity quaternion
        v3 omega(1.0f, 0.0f, 0.0f);               // Rotation around X-axis

        glm::quat result = quat_dot(quat, omega);

        // For identity quaternion rotating around X-axis:
        // Expected: quat_dot = 0.5 * [0.5, 0, 0, 0] (approximately)
        EXPECT_NEAR(result.w, 0.0f, 1e-5f);      // w component
        EXPECT_NEAR(result.x, 0.5f, 1e-5f);      // x component
        EXPECT_NEAR(result.y, 0.0f, 1e-5f);      // y component
        EXPECT_NEAR(result.z, 0.0f, 1e-5f);      // z component
    }

    TEST_F(MathTest, QuatDot_ArbitraryCase) {
        // Test with arbitrary quaternion and angular velocity
        glm::quat quat(0.7071f, 0.7071f, 0.0f, 0.0f);  // 90° rotation around X
        v3 omega(0.0f, 1.0f, 0.0f);                     // Rotation around Y-axis

        glm::quat result = quat_dot(quat, omega);

        // These are computed values - you might want to verify with the Python version
        EXPECT_NEAR(result.w, 0.0f, 1e-4f);
        EXPECT_NEAR(result.x, 0.0f, 1e-4f);
        EXPECT_NEAR(result.y, 0.3535f, 1e-4f);
        EXPECT_NEAR(result.z, 0.3535f, 1e-4f);
    }

    TEST_F(MathTest, QuatDot_NonUnitQuaternion) {
        // Test with non-unit quaternion to verify constraint correction
        glm::quat quat(2.0f, 0.0f, 0.0f, 0.0f);  // Non-unit quaternion
        v3 omega(0.0f, 0.0f, 0.0f);               // Zero angular velocity

        glm::quat result = quat_dot(quat, omega);

        // Should apply constraint correction to drive towards unit length
        // The correction term should be non-zero
        float magnitude = std::sqrt(result.w*result.w + result.x*result.x +
                                  result.y*result.y + result.z*result.z);
        EXPECT_GT(magnitude, 0.0f);  // Should have some correction applied
    }

    TEST_F(MathTest, QuatDot_MultiAxisRotation) {
        // Test with rotation around multiple axes
        glm::quat quat(1.0f, 0.0f, 0.0f, 0.0f);  // Identity quaternion
        v3 omega(1.0f, 2.0f, 3.0f);               // Multi-axis rotation

        glm::quat result = quat_dot(quat, omega);

        // Verify the result has reasonable magnitude
        float magnitude = std::sqrt(result.w*result.w + result.x*result.x +
                                  result.y*result.y + result.z*result.z);
        EXPECT_GT(magnitude, 0.0f);
        EXPECT_LT(magnitude, 10.0f);  // Reasonable upper bound
    }

    // Tests for normalize function
    TEST_F(MathTest, Normalize_UnitVector) {
        // Test with already normalized vector
        v3 input(1.0f, 0.0f, 0.0f);
        v3 result = normalize(input);

        EXPECT_NEAR(result.x, 1.0f, 1e-5f);
        EXPECT_NEAR(result.y, 0.0f, 1e-5f);
        EXPECT_NEAR(result.z, 0.0f, 1e-5f);

        // Verify magnitude is 1
        float magnitude = std::sqrt(result.x*result.x + result.y*result.y + result.z*result.z);
        EXPECT_NEAR(magnitude, 1.0f, 1e-5f);
    }

    TEST_F(MathTest, Normalize_ArbitraryVector) {
        // Test with vector that needs normalization
        v3 input(3.0f, 4.0f, 0.0f);  // Magnitude = 5
        v3 result = normalize(input);

        EXPECT_NEAR(result.x, 0.6f, 1e-5f);   // 3/5
        EXPECT_NEAR(result.y, 0.8f, 1e-5f);   // 4/5
        EXPECT_NEAR(result.z, 0.0f, 1e-5f);

        // Verify magnitude is 1
        float magnitude = std::sqrt(result.x*result.x + result.y*result.y + result.z*result.z);
        EXPECT_NEAR(magnitude, 1.0f, 1e-5f);
    }

    TEST_F(MathTest, Normalize_NegativeComponents) {
        // Test with negative components
        v3 input(-1.0f, -1.0f, -1.0f);
        v3 result = normalize(input);

        float expected_component = -1.0f / std::sqrt(3.0f);
        EXPECT_NEAR(result.x, expected_component, 1e-5f);
        EXPECT_NEAR(result.y, expected_component, 1e-5f);
        EXPECT_NEAR(result.z, expected_component, 1e-5f);

        // Verify magnitude is 1
        float magnitude = std::sqrt(result.x*result.x + result.y*result.y + result.z*result.z);
        EXPECT_NEAR(magnitude, 1.0f, 1e-5f);
    }

    TEST_F(MathTest, Normalize_LargeVector) {
        // Test with large magnitude vector
        v3 input(1000.0f, 2000.0f, 3000.0f);
        v3 result = normalize(input);

        // Verify magnitude is 1
        float magnitude = std::sqrt(result.x*result.x + result.y*result.y + result.z*result.z);
        EXPECT_NEAR(magnitude, 1.0f, 1e-5f);

        // Verify direction is preserved (ratios should be maintained)
        float original_magnitude = std::sqrt(1000.0f*1000.0f + 2000.0f*2000.0f + 3000.0f*3000.0f);
        EXPECT_NEAR(result.x, 1000.0f / original_magnitude, 1e-5f);
        EXPECT_NEAR(result.y, 2000.0f / original_magnitude, 1e-5f);
        EXPECT_NEAR(result.z, 3000.0f / original_magnitude, 1e-5f);
    }

    // Tests for vee_map function
    TEST_F(MathTest, VeeMap_XAxisVector) {
        // Create skew-symmetric matrix for vector (1,0,0)
        // Matrix should be: [ 0  0  0]
        //                   [ 0  0 -1]
        //                   [ 0  1  0]
        m3x3 skew_matrix(0.0f, 0.0f, 0.0f,   // first column
                         0.0f, 0.0f, -1.0f,   // second column
                         0.0f, 1.0f, 0.0f); // third column

        v3 result = vee_map(skew_matrix);

        EXPECT_NEAR(result.x, 1.0f, 1e-5f);
        EXPECT_NEAR(result.y, 0.0f, 1e-5f);
        EXPECT_NEAR(result.z, 0.0f, 1e-5f);
    }

    TEST_F(MathTest, VeeMap_YAxisVector) {
        // Create skew-symmetric matrix for vector (0,1,0)
        // Matrix should be: [ 0  0  1]
        //                   [ 0  0  0]
        //                   [-1  0  0]
        m3x3 skew_matrix(0.0f, 0.0f, 1.0f,  // first column
                         0.0f, 0.0f, 0.0f,   // second column
                         -1.0f, 0.0f, 0.0f);  // third column

        v3 result = vee_map(skew_matrix);

        EXPECT_NEAR(result.x, 0.0f, 1e-5f);
        EXPECT_NEAR(result.y, 1.0f, 1e-5f);
        EXPECT_NEAR(result.z, 0.0f, 1e-5f);
    }

    TEST_F(MathTest, VeeMap_ZAxisVector) {
        // Create skew-symmetric matrix for vector (0,0,1)
        // Matrix should be: [ 0 -1  0]
        //                   [ 1  0  0]
        //                   [ 0  0  0]
        m3x3 skew_matrix(0.0f, -1.0f, 0.0f,   // first column
                         1.0f, 0.0f, 0.0f,  // second column
                         0.0f, 0.0f, 0.0f);  // third column

        v3 result = vee_map(skew_matrix);

        EXPECT_NEAR(result.x, 0.0f, 1e-5f);
        EXPECT_NEAR(result.y, 0.0f, 1e-5f);
        EXPECT_NEAR(result.z, 1.0f, 1e-5f);
    }

    TEST_F(MathTest, VeeMap_ArbitraryVector) {
        // Create skew-symmetric matrix for vector (2,3,4)
        // Matrix should be: [ 0 -4  3]
        //                   [ 4  0 -2]
        //                   [-3  2  0]
        m3x3 skew_matrix(0.0f, -4.0f, 3.0f,  // first column
                         4.0f, 0.0f, -2.0f,  // second column
                         -3.0f, 2.0f, 0.0f); // third column

        v3 result = vee_map(skew_matrix);

        EXPECT_NEAR(result.x, 2.0f, 1e-5f);
        EXPECT_NEAR(result.y, 3.0f, 1e-5f);
        EXPECT_NEAR(result.z, 4.0f, 1e-5f);
    }

    TEST_F(MathTest, VeeMap_ZeroVector) {
        // Create zero skew-symmetric matrix
        m3x3 skew_matrix(0.0f, 0.0f, 0.0f,
                         0.0f, 0.0f, 0.0f,
                         0.0f, 0.0f, 0.0f);

        v3 result = vee_map(skew_matrix);

        EXPECT_NEAR(result.x, 0.0f, 1e-5f);
        EXPECT_NEAR(result.y, 0.0f, 1e-5f);
        EXPECT_NEAR(result.z, 0.0f, 1e-5f);
    }

    // Tests for quaternionToRotationMatrix function
    TEST_F(MathTest, QuaternionToRotationMatrix_Identity) {
        // Identity quaternion should produce identity matrix
        v4 identity_quat(0.0f, 0.0f, 0.0f, 1.0f);  // [x,y,z,w] format
        m3x3 result = quaternionToRotationMatrix(identity_quat);

        // Check diagonal elements
        EXPECT_NEAR(result[0][0], 1.0f, 1e-5f);
        EXPECT_NEAR(result[1][1], 1.0f, 1e-5f);
        EXPECT_NEAR(result[2][2], 1.0f, 1e-5f);

        // Check off-diagonal elements
        EXPECT_NEAR(result[0][1], 0.0f, 1e-5f);
        EXPECT_NEAR(result[0][2], 0.0f, 1e-5f);
        EXPECT_NEAR(result[1][0], 0.0f, 1e-5f);
        EXPECT_NEAR(result[1][2], 0.0f, 1e-5f);
        EXPECT_NEAR(result[2][0], 0.0f, 1e-5f);
        EXPECT_NEAR(result[2][1], 0.0f, 1e-5f);
    }

    TEST_F(MathTest, QuaternionToRotationMatrix_90DegreeX) {
        // 90-degree rotation around X-axis
        v4 quat_x(0.7071f, 0.0f, 0.0f, 0.7071f);  // [x,y,z,w] format
        m3x3 result = quaternionToRotationMatrix(quat_x);

        // Expected matrix for 90° rotation around X:
        // [1  0  0]
        // [0  0 -1]
        // [0  1  0]
        EXPECT_NEAR(result[0][0], 1.0f, 1e-4f);
        EXPECT_NEAR(result[0][1], 0.0f, 1e-4f);
        EXPECT_NEAR(result[0][2], 0.0f, 1e-4f);
        EXPECT_NEAR(result[1][0], 0.0f, 1e-4f);
        EXPECT_NEAR(result[1][1], 0.0f, 1e-4f);
        EXPECT_NEAR(result[1][2], 1.0f, 1e-4f);
        EXPECT_NEAR(result[2][0], 0.0f, 1e-4f);
        EXPECT_NEAR(result[2][1], -1.0f, 1e-4f);
        EXPECT_NEAR(result[2][2], 0.0f, 1e-4f);
    }

    TEST_F(MathTest, QuaternionToRotationMatrix_90DegreeY) {
        // 90-degree rotation around Y-axis
        v4 quat_y(0.0f, 0.7071f, 0.0f, 0.7071f);  // [x,y,z,w] format
        m3x3 result = quaternionToRotationMatrix(quat_y);

        // Expected matrix for 90° rotation around Y:
        // [ 0  0  1]
        // [ 0  1  0]
        // [-1  0  0]
        EXPECT_NEAR(result[0][0], 0.0f, 1e-4f);
        EXPECT_NEAR(result[0][1], 0.0f, 1e-4f);
        EXPECT_NEAR(result[0][2], -1.0f, 1e-4f);
        EXPECT_NEAR(result[1][0], 0.0f, 1e-4f);
        EXPECT_NEAR(result[1][1], 1.0f, 1e-4f);
        EXPECT_NEAR(result[1][2], 0.0f, 1e-4f);
        EXPECT_NEAR(result[2][0], 1.0f, 1e-4f);
        EXPECT_NEAR(result[2][1], 0.0f, 1e-4f);
        EXPECT_NEAR(result[2][2], 0.0f, 1e-4f);
    }

    TEST_F(MathTest, QuaternionToRotationMatrix_OrthogonalityCheck) {
        // Test that the result is indeed a rotation matrix (orthogonal)
        v4 arbitrary_quat(0.1f, 0.2f, 0.3f, 0.9274f);  // Normalized quaternion
        m3x3 result = quaternionToRotationMatrix(arbitrary_quat);

        // Check that R * R^T = I (orthogonality test)
        m3x3 transpose = glm::transpose(result);
        m3x3 product = result * transpose;

        // Check diagonal elements are close to 1
        EXPECT_NEAR(product[0][0], 1.0f, 1e-4f);
        EXPECT_NEAR(product[1][1], 1.0f, 1e-4f);
        EXPECT_NEAR(product[2][2], 1.0f, 1e-4f);

        // Check off-diagonal elements are close to 0
        EXPECT_NEAR(product[0][1], 0.0f, 1e-4f);
        EXPECT_NEAR(product[0][2], 0.0f, 1e-4f);
        EXPECT_NEAR(product[1][0], 0.0f, 1e-4f);
        EXPECT_NEAR(product[1][2], 0.0f, 1e-4f);
        EXPECT_NEAR(product[2][0], 0.0f, 1e-4f);
        EXPECT_NEAR(product[2][1], 0.0f, 1e-4f);
    }

    TEST_F(MathTest, QuaternionToRotationMatrix_DeterminantCheck) {
        // Test that the determinant is 1 (proper rotation)
        v4 arbitrary_quat(0.1f, 0.2f, 0.3f, 0.9274f);  // Normalized quaternion
        m3x3 result = quaternionToRotationMatrix(arbitrary_quat);

        float determinant = glm::determinant(result);
        EXPECT_NEAR(determinant, 1.0f, 1e-4f);
    }
}