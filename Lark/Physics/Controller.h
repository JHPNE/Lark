#pragma once

#include "DroneTypes.h"
#include "ControllerTypes.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <optional>
#include <string>

namespace lark::drones {

/**
 * @brief SE3 geometric controller implementation
 * @details Provides hierarchical control of position/velocity and attitude
 *          based on differential geometric methods on SE(3)
 */
class Controller {
public:
    /**
     * @brief Constructs controller with specified gains and parameters
     *
     * @param vehicleParams Physical parameters of the vehicle
     * @param gains Control gains structure
     * @throws std::invalid_argument if parameters are invalid
     *
     * @pre vehicleParams must have positive mass and valid inertia
     * @pre gains must pass validation
     */
    explicit Controller(
        const InertiaProperties& vehicleParams,
        const ControllerGains& gains = ControllerGains{});

    /**
     * @brief Computes control commands based on current state and desired trajectory
     *
     * @param mode Control abstraction mode
     * @param state Current validated vehicle state
     * @param flatOutput Desired flat outputs from trajectory
     * @return Control inputs for the specified mode
     * @throws std::invalid_argument if inputs are invalid
     * @throws std::runtime_error if computation fails
     *
     * @pre state must be valid according to vehicle constraints
     * @pre flatOutput must pass validation
     * @pre mode must be supported by vehicle
     */
    [[nodiscard]] ControlInput computeControl(
        ControlMode mode,
        const DroneState& state,
        const FlatOutput& flatOutput);

private:
    const InertiaProperties vehicleParams;
    const ControllerGains gains;

    /**
     * @brief Computes desired force vector in world frame
     *
     * @param state Current vehicle state
     * @param flatOutput Desired flat outputs
     * @return Desired force vector (N)
     * @throws std::runtime_error if computation fails
     */
    [[nodiscard]] glm::vec3 computeDesiredForce(
        const DroneState& state,
        const FlatOutput& flatOutput) const;

    /**
     * @brief Computes desired orientation matrix
     *
     * @param forceDesired Desired force vector
     * @param yawDesired Desired yaw angle
     * @return Desired rotation matrix
     * @throws std::runtime_error if computation fails
     */
    [[nodiscard]] glm::mat3 computeDesiredRotation(
        const glm::vec3& forceDesired,
        float yawDesired) const;

    /**
     * @brief Computes attitude error vector from current and desired orientations
     *
     * @param R_current Current rotation matrix
     * @param R_desired Desired rotation matrix
     * @return Error vector in body frame
     * @throws std::runtime_error if matrices are invalid
     */
    [[nodiscard]] glm::vec3 computeAttitudeError(
        const glm::mat3& R_current,
        const glm::mat3& R_desired) const;

    /**
     * @brief Computes commanded body moments
     *
     * @param attitudeError Attitude error vector
     * @param angularVel Current angular velocity
     * @param angularVelDes Desired angular velocity (default 0)
     * @return Commanded moment vector (N⋅m)
     * @throws std::runtime_error if computation fails
     */
    [[nodiscard]] glm::vec3 computeCommandMoments(
        const glm::vec3& attitudeError,
        const glm::vec3& angularVel,
        const glm::vec3& angularVelDes = glm::vec3(0.0f)) const;

    /**
     * @brief Validates that rotation matrix is valid SO(3) element
     *
     * @param R Rotation matrix to validate
     * @return std::nullopt if valid, error message if invalid
     */
    [[nodiscard]] static std::optional<std::string> validateRotationMatrix(
        const glm::mat3& R);
};

} // namespace lark::drones