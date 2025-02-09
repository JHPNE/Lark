#pragma once

#include <glm/glm.hpp>
#include <optional>
#include <string>

namespace lark::drones {

/**
 * @brief Flat output representation for differential flatness
 * @details Contains position and derivatives up to snap, plus yaw
 */
struct FlatOutput {
    glm::vec3 position;      ///< Desired position in world frame (m)
    glm::vec3 velocity;      ///< Desired velocity in world frame (m/s)
    glm::vec3 acceleration;  ///< Desired acceleration in world frame (m/s^2)
    glm::vec3 jerk;         ///< Desired jerk in world frame (m/s^3)
    glm::vec3 snap;         ///< Desired snap in world frame (m/s^4)
    float yaw;              ///< Desired yaw angle (rad)
    float yawRate;          ///< Desired yaw rate (rad/s)

    /**
     * @brief Validates flat output values
     * @return std::nullopt if valid, error message if invalid
     */
    [[nodiscard]] std::optional<std::string> validate() const {
        // Check for non-finite values
        auto checkVector = [](const glm::vec3& v, const char* name) -> std::optional<std::string> {
            if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z)) {
                return std::string(name) + " contains non-finite values";
            }
            return std::nullopt;
        };

        if (auto err = checkVector(position, "Position")) return err;
        if (auto err = checkVector(velocity, "Velocity")) return err;
        if (auto err = checkVector(acceleration, "Acceleration")) return err;
        if (auto err = checkVector(jerk, "Jerk")) return err;
        if (auto err = checkVector(snap, "Snap")) return err;

        if (!std::isfinite(yaw)) return "Yaw is not finite";
        if (!std::isfinite(yawRate)) return "Yaw rate is not finite";

        return std::nullopt;
    }
};

/**
 * @brief Gain structure for SE3 geometric controller
 */
struct ControllerGains {
    glm::vec3 kPosition{6.5f, 6.5f, 15.0f};  ///< Position gains
    glm::vec3 kVelocity{4.0f, 4.0f, 9.0f};   ///< Velocity gains
    float kAttitudeP{544.0f};                 ///< Attitude proportional gain
    float kAttitudeD{46.64f};                 ///< Attitude derivative gain
    float kVelocityP{0.65f};                  ///< Velocity P gain for velocity mode
    
    /**
     * @brief Validates gain values
     * @return std::nullopt if valid, error message if invalid
     */
    [[nodiscard]] std::optional<std::string> validate() const {
        // Check for non-positive gains
        auto checkGainVector = [](const glm::vec3& gains, const char* name) 
            -> std::optional<std::string> {
            if (gains.x <= 0.0f || gains.y <= 0.0f || gains.z <= 0.0f) {
                return std::string(name) + " gains must be positive";
            }
            return std::nullopt;
        };

        if (auto err = checkGainVector(kPosition, "Position")) return err;
        if (auto err = checkGainVector(kVelocity, "Velocity")) return err;
        
        if (kAttitudeP <= 0.0f) return "Attitude P gain must be positive";
        if (kAttitudeD <= 0.0f) return "Attitude D gain must be positive";
        if (kVelocityP <= 0.0f) return "Velocity P gain must be positive";

        return std::nullopt;
    }
};

} // namespace lark::drones