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
}