#include "Controller.h"
#include <sstream>
#include <stdexcept>

namespace lark::drones {
  Controller::Controller(
      const InertiaProperties& params,
      const ControllerGains& gains)
      : vehicleParams(params), gains(gains) {

    // Validate vehicle parameters
    if (params.mass <= 0.0f) {
      throw std::invalid_argument("Vehicle mass must be positive");
    }

    // Validate gains
    if (auto error = gains.validate()) {
      throw std::invalid_argument(error.value());
    }
  }

  ControlInput Controller::computeControl(
    ControlMode mode,
    const DroneState& state,
    const FlatOutput& flatOutput) {

        ControlInput control;
        control.mode = mode;

        try {
            // Compute desired force vector in world frame
            glm::vec3 forceDesired = computeDesiredForce(state, flatOutput);

            // Current orientation
            glm::mat3 R_current = glm::mat3_cast(state.orientation);
            if (auto error = validateRotationMatrix(R_current)) {
                throw std::runtime_error("Invalid current rotation: " + error.value());
            }

            // Body z-axis
            glm::vec3 b3 = R_current[2];

            // Compute thrust as projection of desired force onto body z-axis
            float thrust = glm::dot(forceDesired, b3);
            control.collectiveThrust = thrust;

            switch (mode) {
                case ControlMode::COLLECTIVE_THRUST_ATTITUDE: {
                    // Compute desired orientation
                    glm::mat3 R_desired = computeDesiredRotation(
                        forceDesired, flatOutput.yaw);

                    if (auto error = validateRotationMatrix(R_desired)) {
                        throw std::runtime_error("Invalid desired rotation: " + error.value());
                    }

                    // Convert to quaternion
                    control.targetAttitude = glm::quat_cast(R_desired);
                    break;
                }

                case ControlMode::COLLECTIVE_THRUST_BODY_MOMENTS: {
                    // Compute desired orientation
                    glm::mat3 R_desired = computeDesiredRotation(
                        forceDesired, flatOutput.yaw);

                    if (auto error = validateRotationMatrix(R_desired)) {
                        throw std::runtime_error("Invalid desired rotation: " + error.value());
                    }

                    // Compute attitude error
                    glm::vec3 attitudeError = computeAttitudeError(R_current, R_desired);

                    // Compute moments with yaw rate feedforward
                    glm::vec3 angularVelDesired(0.0f, 0.0f, flatOutput.yawRate);
                    control.bodyMoments = computeCommandMoments(
                        attitudeError, state.angular_velocity, angularVelDesired);
                    break;
                }

                case ControlMode::VELOCITY: {
                    // Simple P control on velocity error
                    glm::vec3 velocityError = state.velocity - flatOutput.velocity;
                    glm::vec3 desiredAccel = -gains.kVelocityP * velocityError;

                    // Add gravity compensation
                    glm::vec3 desiredForce = vehicleParams.mass * (
                        desiredAccel + glm::vec3(0.0f, 0.0f, 9.81f));

                    control.targetVelocity = flatOutput.velocity;
                    break;
                }

                default:
                    throw std::invalid_argument("Unsupported control mode");
            }

        } catch (const std::exception& e) {
            throw std::runtime_error(
                std::string("Control computation failed: ") + e.what());
        }

        return control;
    }
    glm::vec3 Controller::computeDesiredForce(
    const DroneState& state,
    const FlatOutput& flatOutput) const {

        // Compute errors
        glm::vec3 posError = state.position - flatOutput.position;
        glm::vec3 velError = state.velocity - flatOutput.velocity;

        // PD control + feedforward
        return vehicleParams.mass * (
            -gains.kPosition * posError
            - gains.kVelocity * velError
            + flatOutput.acceleration
            + glm::vec3(0.0f, 0.0f, 9.81f));
    }

    glm::mat3 Controller::computeDesiredRotation(
        const glm::vec3& forceDesired,
        float yawDesired) const {

        // Desired body z-axis aligned with force
        glm::vec3 b3_des = glm::normalize(forceDesired);

        // Construct desired x-axis based on yaw
        glm::vec3 c1_des(
            std::cos(yawDesired),
            std::sin(yawDesired),
            0.0f);

        // Complete right-handed coordinate system
        glm::vec3 b2_des = glm::normalize(
            glm::cross(b3_des, c1_des));
        glm::vec3 b1_des = glm::cross(b2_des, b3_des);

        // Construct rotation matrix
        return glm::mat3(b1_des, b2_des, b3_des);
    }

    glm::vec3 Controller::computeAttitudeError(
        const glm::mat3& R_current,
        const glm::mat3& R_desired) const {

        // Compute SO(3) error metric
        glm::mat3 R_error = 0.5f * (
            glm::transpose(R_desired) * R_current -
            glm::transpose(R_current) * R_desired);

        // Extract vector component
        return glm::vec3(
            -R_error[1][2],
            R_error[0][2],
            -R_error[0][1]);
    }

    glm::vec3 Controller::computeCommandMoments(
        const glm::vec3& attitudeError,
        const glm::vec3& angularVel,
        const glm::vec3& angularVelDes) const {

        glm::mat3 inertia = vehicleParams.getInertiaMatrix();

        // PD control on attitude
        glm::vec3 commandedMoments = inertia * (
            -gains.kAttitudeP * attitudeError
            - gains.kAttitudeD * (angularVel - angularVelDes));

        // Add gyroscopic compensation
        commandedMoments += glm::cross(
            angularVel,
            inertia * angularVel);

        return commandedMoments;
    }

    std::optional<std::string> Controller::validateRotationMatrix(
        const glm::mat3& R) {

        constexpr float ORTHOGONALITY_TOLERANCE = 1e-6f;
        constexpr float DETERMINANT_TOLERANCE = 1e-6f;

        // Check orthogonality
        glm::mat3 RRT = R * glm::transpose(R);
        glm::mat3 I = glm::mat3(1.0f);

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                if (std::abs(RRT[i][j] - I[i][j]) > ORTHOGONALITY_TOLERANCE) {
                    return "Matrix is not orthogonal";
                }
            }
        }

        // Check proper rotation (determinant = 1)
        float det = glm::determinant(R);
        if (std::abs(det - 1.0f) > DETERMINANT_TOLERANCE) {
            return "Matrix determinant is not 1";
        }

        return std::nullopt;
    }
}
