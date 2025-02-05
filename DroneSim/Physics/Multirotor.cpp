#include "Multirotor.h"

#include "Utils/ErrorHandling.h"
#include "Utils/Logger.h"
#include <chrono>
#include <random>
#include <stdexcept>

namespace lark::drones {
  DroneState Multirotor::step(const DroneState& state, const ControlInput& control, float timeStep) {
    if (timeStep <= 0.0f) {
      throw std::invalid_argument("Time step must be positive");
    }

    if (auto stateError = validateState(state)) {
      throw std::invalid_argument("Invalid initial state: " + stateError.value());
    }

    if (auto controlError = validateControl(control)) {
      throw std::invalid_argument("Invalid control input: " + controlError.value());
    }

    // Compute commanded motor speeds with validation
    std::vector<float> cmdRotorSpeeds;
    try {
       cmdRotorSpeeds = computeCommandedMotorSpeeds(state, control);
    } catch (const std::exception& e) {
       throw std::runtime_error("Failed to compute motor speeds: " + std::string(e.what()));
    }

    DroneState nextState = state;

    try {
      auto [linearAccel, angularAccel] = computeStateDerivatives(state, control, timeStep);

      // Position Integration (simple euler for now - enhance to rk4 later)
      nextState.position += state.velocity * timeStep;

      // Vel Integ
      nextState.velocity += linearAccel * timeStep;

      // orientation integration using quaternion kinematics
      glm::quat quatDot = computeQuaternionDerivative(state.orientation, state.angular_velocity);
      nextState.orientation += quatDot * timeStep;

      // Normalize quaternion to maintain unit length
      nextState.orientation = glm::normalize(nextState.orientation);

      nextState.angular_velocity += angularAccel * timeStep;

      // Rotor speed dynamics first order response

      for (size_t i = 0; i < rotors.size(); ++i) {
        float rotorAccel = (1.0f / motorProps.responseTime) * (cmdRotorSpeeds[i] - state.rotor_speeds[i]);
        nextState.rotor_speeds[i] += rotorAccel * timeStep;
      }

        // Add motor noise if enabled
        if (motorProps.noiseStdDev > 0.0f) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::normal_distribution<float> noise(0.0f, motorProps.noiseStdDev);

            for (size_t i = 0; i < rotors.size(); ++i) {
                nextState.rotor_speeds[i] += noise(gen) * std::sqrt(timeStep);
            }
        }

        // Enforce rotor speed limits
        for (size_t i = 0; i < rotors.size(); ++i) {
            nextState.rotor_speeds[i] = glm::clamp(
                nextState.rotor_speeds[i],
                rotors[i].minSpeed,
                rotors[i].maxSpeed
            );
        }

    } catch (const std::exception& e) {
        throw std::runtime_error("State integration failed: " + std::string(e.what()));
    }

      // POST-CONDITIONS
      if (auto stateError = validateState(nextState)) {
          throw std::runtime_error("Invalid state after integration: " + stateError.value());
      }

      return nextState;
  }

  std::vector<float> Multirotor::computeCommandedMotorSpeeds(
    const DroneState& state,
    const ControlInput& control) {

    // Pre-validate inputs
    if (auto stateError = validateState(state)) {
        throw std::invalid_argument("Invalid state: " + stateError.value());
    }
    if (auto controlError = validateControl(control)) {
        throw std::invalid_argument("Invalid control: " + controlError.value());
    }

    std::vector<float> cmdMotorSpeeds;
    cmdMotorSpeeds.reserve(rotors.size());

    switch (control.mode) {
        case ControlMode::MOTOR_SPEEDS: {
            // Direct motor speed control
            cmdMotorSpeeds = control.motorSpeeds;
            break;
        }

        case ControlMode::MOTOR_THRUSTS: {
            // Convert commanded thrusts to motor speeds
            cmdMotorSpeeds.resize(rotors.size());
            for (size_t i = 0; i < rotors.size(); ++i) {
                float thrust = control.motorThrusts[i];
                float speed = std::copysign(
                    std::sqrt(std::abs(thrust / rotors[i].thrustCoeff)),
                    thrust
                );
                cmdMotorSpeeds[i] = speed;
            }
            break;
        }

        case ControlMode::COLLECTIVE_THRUST_BODY_RATES: {
            // Compute error between desired and actual body rates
            glm::vec3 rateError = state.angular_velocity - control.bodyRates;

            // Compute desired angular acceleration (P control)
            glm::vec3 desiredAngularAccel = -motorProps.bodyRateGain * rateError;

            // Convert to body moments using inertia matrix
            glm::vec3 commandedMoments = inertialProps.getInertiaMatrix() * desiredAngularAccel;

            // Combine thrust and moments for allocation
            glm::vec4 thrustMoments(
                control.collectiveThrust,
                commandedMoments.x,
                commandedMoments.y,
                commandedMoments.z
            );

            // Convert to motor speeds using allocation matrix
            glm::vec4 motorForces = thrustMomentToForce * thrustMoments;
            cmdMotorSpeeds.resize(rotors.size());
            for (size_t i = 0; i < rotors.size(); ++i) {
                float speed = std::copysign(
                    std::sqrt(std::abs(motorForces[i] / rotors[i].thrustCoeff)),
                    motorForces[i]
                );
                cmdMotorSpeeds[i] = speed;
            }
            break;
        }

        case ControlMode::COLLECTIVE_THRUST_BODY_MOMENTS: {
            // Direct thrust and moment allocation
            glm::vec4 thrustMoments(
                control.collectiveThrust,
                control.bodyMoments.x,
                control.bodyMoments.y,
                control.bodyMoments.z
            );

            // Convert to motor speeds using allocation matrix
            glm::vec4 motorForces = thrustMomentToForce * thrustMoments;
            cmdMotorSpeeds.resize(rotors.size());
            for (size_t i = 0; i < rotors.size(); ++i) {
                float speed = std::copysign(
                    std::sqrt(std::abs(motorForces[i] / rotors[i].thrustCoeff)),
                    motorForces[i]
                );
                cmdMotorSpeeds[i] = speed;
            }
            break;
        }

        case ControlMode::COLLECTIVE_THRUST_ATTITUDE: {
            // Extract current rotation matrix from quaternion
            glm::mat3 R = glm::mat3_cast(state.orientation);
            glm::mat3 R_des = glm::mat3_cast(control.targetAttitude);

            // Compute attitude error (SO(3) error metric)
            glm::mat3 errorMatrix = 0.5f * (
                glm::transpose(R_des) * R -
                glm::transpose(R) * R_des
            );

            glm::vec3 attitudeError(
                -errorMatrix[1][2],
                errorMatrix[0][2],
                -errorMatrix[0][1]
            );

            // Compute commanded moment using PD control
            glm::vec3 commandedMoments = inertialProps.getInertiaMatrix() * (
                -motorProps.attitudePGain * attitudeError -
                motorProps.attitudeDGain * state.angular_velocity
            );

            // Add gyroscopic compensation
            commandedMoments += glm::cross(
                state.angular_velocity,
                inertialProps.getInertiaMatrix() * state.angular_velocity
            );

            // Combine thrust and moments for allocation
            glm::vec4 thrustMoments(
                control.collectiveThrust,
                commandedMoments.x,
                commandedMoments.y,
                commandedMoments.z
            );

            // Convert to motor speeds using allocation matrix
            glm::vec4 motorForces = thrustMomentToForce * thrustMoments;
            cmdMotorSpeeds.resize(rotors.size());
            for (size_t i = 0; i < rotors.size(); ++i) {
                float speed = std::copysign(
                    std::sqrt(std::abs(motorForces[i] / rotors[i].thrustCoeff)),
                    motorForces[i]
                );
                cmdMotorSpeeds[i] = speed;
            }
            break;
        }

        case ControlMode::VELOCITY: {
            // Compute velocity error in world frame
            glm::vec3 velocityError = state.velocity - control.targetVelocity;

            // P control for velocity error
            glm::vec3 desiredAccel = -motorProps.velocityGain * velocityError;

            // Add gravity compensation
            constexpr float STANDARD_GRAVITY = 9.81f;
            glm::vec3 desiredForce = inertialProps.mass * (
                desiredAccel + glm::vec3(0.0f, 0.0f, STANDARD_GRAVITY)
            );

            // Extract current rotation
            glm::mat3 R = glm::mat3_cast(state.orientation);
            glm::vec3 b3 = R[2]; // Third column is z-axis

            // Project desired force onto body z-axis for thrust
            float collectiveThrust = glm::dot(desiredForce, b3);

            // Compute desired orientation
            glm::vec3 b3_des = glm::normalize(desiredForce);
            glm::vec3 c1_des(1.0f, 0.0f, 0.0f);
            glm::vec3 b2_des = glm::normalize(
                glm::cross(b3_des, c1_des)
            );
            glm::vec3 b1_des = glm::cross(b2_des, b3_des);

            // Construct desired rotation matrix
            glm::mat3 R_des(b1_des, b2_des, b3_des);

            // Compute orientation error
            glm::mat3 errorMatrix = 0.5f * (
                glm::transpose(R_des) * R -
                glm::transpose(R) * R_des
            );

            glm::vec3 attitudeError(
                -errorMatrix[1][2],
                errorMatrix[0][2],
                -errorMatrix[0][1]
            );

            // Compute commanded moments (PD control + gyroscopic compensation)
            glm::vec3 commandedMoments = inertialProps.getInertiaMatrix() * (
                -motorProps.attitudePGain * attitudeError -
                motorProps.attitudeDGain * state.angular_velocity
            ) + glm::cross(
                state.angular_velocity,
                inertialProps.getInertiaMatrix() * state.angular_velocity
            );

            // Combine thrust and moments for allocation
            glm::vec4 thrustMoments(
                collectiveThrust,
                commandedMoments.x,
                commandedMoments.y,
                commandedMoments.z
            );

            // Convert to motor speeds using allocation matrix
            glm::vec4 motorForces = thrustMomentToForce * thrustMoments;
            cmdMotorSpeeds.resize(rotors.size());
            for (size_t i = 0; i < rotors.size(); ++i) {
                float speed = std::copysign(
                    std::sqrt(std::abs(motorForces[i] / rotors[i].thrustCoeff)),
                    motorForces[i]
                );
                cmdMotorSpeeds[i] = speed;
            }
            break;
        }

        default:
            throw std::invalid_argument("Unsupported control mode");
    }

    // Enforce motor speed limits
    for (size_t i = 0; i < cmdMotorSpeeds.size(); ++i) {
        cmdMotorSpeeds[i] = glm::clamp(
            cmdMotorSpeeds[i],
            rotors[i].minSpeed,
            rotors[i].maxSpeed
        );
    }

    return cmdMotorSpeeds;
  }

  std::pair<glm::vec3, glm::vec3> Multirotor::computeStateDerivatives(const DroneState& state, const ControlInput& control, float timeStep) {
    // Input validation
    if (timeStep <= 0.0f) {
      throw std::invalid_argument("Time step must be positive");
    }

    if (auto validationError = validateState(state)) {
      throw std::invalid_argument(validationError.value());
    }

    if (auto validationError = validateControl(control)) {
      throw std::invalid_argument(validationError.value());
    }

    const auto& rotor_speeds = state.rotor_speeds;
    const auto& inertial_velocity = state.velocity;
    const auto& wind_velocity = state.wind;
    const auto& bodyRates = state.angular_velocity;

    // compute rotation matrix from quaternion
    glm::mat3 R = glm::mat3_cast(state.orientation);

    glm::vec3 bodyAirspeedVector = glm::transpose(R) * (inertial_velocity - wind_velocity);

    auto [bodyForce, bodyMoment] = computeBodyWrench(bodyRates, rotor_speeds, bodyAirspeedVector);

    glm::vec3 inertialForce = R * bodyForce;

    // move to env
    const glm::vec3 gravity(0.0f, 0.0f, -9.81f);

    // F = ma → a = F/m + g
    glm::vec3 linearAccel = (inertialForce /inertialProps.mass) + gravity;

    glm::vec3 angularAccel = inverseInertia * (
      bodyMoment - glm::cross(bodyRates, inertialProps.getInertiaMatrix() * bodyRates)
    );

    return {linearAccel, angularAccel};
  }

  std::pair<glm::vec3, glm::vec3> Multirotor::computeBodyWrench(
      const glm::vec3& bodyRates,
      const std::vector<float>& rotorSpeeds,
      const glm::vec3& bodyAirspeed) const {

    if (rotorSpeeds.size() != rotors.size()) {
      throw std::invalid_argument("rotorSpeeds.size() != rotors.size()");
    }

    glm::vec3 totalForce(0.0f);
    glm::vec3 totalMoment(0.0f);

    for (size_t i = 0; i < rotors.size(); ++i) {
      const auto& rotor = rotors[i];

      glm::vec3 localAirspeed = bodyAirspeed + glm::cross(bodyRates, rotor.position);

      float thrust = rotor.thrustCoeff * rotorSpeeds[i] * rotorSpeeds[i];
      glm::vec3 thrustForce(0.0f, 0.0f, thrust);

      // Add aerodynamics effects if enabled
      if (aeroProps.enableAerodynamics) {
        float rotorSpeed = rotorSpeeds[i];

        // Drag
        auto x = rotor.dragCoeff * localAirspeed.x;
        auto y = rotor.dragCoeff * localAirspeed.y;
        auto z = rotor.inflowCoeff * localAirspeed.z;
        glm::vec3 rotorDrag = -rotorSpeed * glm::vec3(x, y, z);


        // Flapping moments
        glm::vec3 flapMoment = -rotor.flapCoeff * rotorSpeed * glm::cross(localAirspeed, glm::vec3(.0f, .0f, 1.0f));

        totalMoment += flapMoment;
        thrustForce += rotorDrag;
      }

      // Add thrust force
      totalForce += thrustForce;

      // Add thrust moment
      totalMoment += glm::cross(rotor.position, thrustForce);

      // Add motor torque
      totalMoment.z += rotor.direction * rotor.torqueCoeff *
          rotorSpeeds[i] * rotorSpeeds[i];
    }

    // Add parasitic drag if enabled
    if (aeroProps.enableAerodynamics) {
      float airspeedMag = glm::length(bodyAirspeed);
      totalForce += -airspeedMag * aeroProps.getDragMatrix() * bodyAirspeed;
    }

    return {totalForce, totalMoment};
  }

  glm::mat3 Multirotor::hatMap(const glm::vec3& v) {
    return glm::mat3(
          0.0f, -v.z, v.y,
          v.z, 0.0f, -v.x,
          -v.y, v.x, 0.0f
    );
  }

  glm::quat Multirotor::computeQuaternionDerivative(const glm::quat &quat,
                                                    const glm::vec3 &omega) {
    // PRE-CONDITIONS
    if (!std::isfinite(glm::length(omega))) {
        throw std::invalid_argument("Angular velocity contains non-finite values");
    }

    // Ensure input quaternion is normalized (within numerical precision)
    constexpr float QUAT_NORM_TOLERANCE = 1e-6f;
    const float normError = std::abs(glm::length(quat) - 1.0f);
    if (normError > QUAT_NORM_TOLERANCE) {
        throw std::invalid_argument("Input quaternion is not normalized");
    }

    // Extract quaternion components for clarity
    // Note: GLM quaternion order is (x,y,z,w)
    const float qx = quat.x;
    const float qy = quat.y;
    const float qz = quat.z;
    const float qw = quat.w;

    // Construct the G matrix as defined in Graf's paper
    // G = [[ q3,  q2, -q1, -q0],
    //      [-q2,  q3,  q0, -q1],
    //      [ q1, -q0,  q3, -q2]]
    const glm::mat3x4 G(
         qw,  qz, -qy, -qx,
        -qz,  qw,  qx, -qy,
         qy, -qx,  qw, -qz
    );

    // Compute quaternion derivative: qdot = 0.5 * G^T * omega
    glm::vec4 quatDot(
        0.5f * (-omega.x*qx - omega.y*qy - omega.z*qz),  // w component
        0.5f * ( omega.x*qw + omega.z*qy - omega.y*qz),  // x component
        0.5f * ( omega.y*qw - omega.z*qx + omega.x*qz),  // y component
        0.5f * ( omega.z*qw + omega.y*qx - omega.x*qy)   // z component
    );

    // Augment quaternion derivative to maintain unit norm constraint
    // This helps prevent numerical drift in the quaternion normalization
    const float quatError = glm::dot(glm::vec4(qx, qy, qz, qw),
                                   glm::vec4(qx, qy, qz, qw)) - 1.0f;
    const glm::vec4 quatErrorGradient = 2.0f * glm::vec4(qx, qy, qz, qw);
    quatDot -= quatError * quatErrorGradient;

    // POST-CONDITIONS
    if (!std::isfinite(glm::length(glm::quat(quatDot.w, quatDot.x, quatDot.y, quatDot.z)))) {
        throw std::runtime_error("Quaternion derivative computation produced non-finite values");
    }


    return glm::quat(quatDot.w, quatDot.x, quatDot.y, quatDot.z);
  }

}