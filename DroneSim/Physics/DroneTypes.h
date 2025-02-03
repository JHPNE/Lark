
#include <array>
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace lark::drones {

  /**
   * @brief Control abstraction levels for drone control
   */
  enum class ControlMode : uint8_t {
    MOTOR_SPEEDS,                    ///< Direct motor speed control
    MOTOR_THRUSTS,                   ///< Individual motor thrust control
    COLLECTIVE_THRUST_BODY_RATES,    ///< Collective thrust and body rates
    COLLECTIVE_THRUST_BODY_MOMENTS,  ///< Collective thrust and body moments
    COLLECTIVE_THRUST_ATTITUDE,      ///< Collective thrust and attitude
    VELOCITY,                        ///< Velocity control
    ACCELERATION                     ///< Acceleration control
  };


  /**
   * @brief Complete state representation of a drone
   */
  struct DroneState {
    glm::vec3 position;           ///< Inertial position (m)
    glm::vec3 velocity;           ///< Inertial velocity (m/s) the world velocity P gain
    glm::quat orientation;        ///< Orientation quaternion [x,y,z,w]
    glm::vec3 angular_velocity;    ///< Body rates (rad/s)
    glm::vec3 wind;               ///< Wind vector (m/s)
    std::vector<float> rotor_speeds; ///< Current rotor speeds (rad/s)

    /**
     * @brief Validates the state vector dimensions
     * @param expectedRotors Expected number of rotors
     * @return True if dimensions are valid
     */
    [[nodiscard]] bool validateDimensions(size_t expectedRotors) const {
      return rotor_speeds.size() == expectedRotors;
    }
  };

  struct InertiaProperties {
    float mass; ///< Mass of the drone (kg)

    // Full inertia tensor components
    float Ixx;                    ///< Moment of inertia around x-axis (kg⋅m²)
    float Iyy;                    ///< Moment of inertia around y-axis (kg⋅m²)
    float Izz;                    ///< Moment of inertia around z-axis (kg⋅m²)
    float Ixy;                    ///< Product of inertia xy (kg⋅m²)
    float Iyz;                    ///< Product of inertia yz (kg⋅m²)
    float Ixz;                    ///< Product of inertia xz (kg⋅m²)

    /**
     * @brief Constructs the full 3x3 inertia matrix
     * @return 3x3 inertia matrix
     */
    [[nodiscard]] glm::mat3 getInertiaMatrix() const {
      return glm::mat3(
          Ixx, Ixy, Ixz,
          Ixy, Iyy, Iyz,
          Ixz, Iyz, Izz
      );
    }
  };

  /**
   * @brief Comprehensive rotor parameters
   */
  struct RotorParameters {
    float thrustCoeff;           ///< k_eta: thrust coefficient N/(rad/s)^2
    float torqueCoeff;           ///< k_m: yaw moment coefficient Nm/(rad/s)^2
    float dragCoeff;             ///< k_d: rotor drag coefficient N/(rad*m/s^2)
    float inflowCoeff;           ///< k_z: induced inflow coefficient N/(rad*m/s^2)
    float flapCoeff;             ///< k_flap: Flapping moment coefficient Nm/(rad*m/s^2)
    glm::vec3 position;          ///< Position relative to CoM (m)
    int direction;               ///< Rotation direction (1 or -1)
    float minSpeed;              ///< Minimum rotor speed (rad/s)
    float maxSpeed;              ///< Maximum rotor speed (rad/s)

    /**
     * @brief Validates rotor parameters
     * @return std::nullopt if valid, error message if invalid
     */
    [[nodiscard]] std::optional<std::string> validate() const {
      if (thrustCoeff <= 0.0f) return "Invalid thrust coefficient";
      if (minSpeed < 0.0f) return "Invalid minimum speed";
      if (maxSpeed <= minSpeed) return "Invalid maximum speed";
      if (direction != 1 && direction != -1) return "Invalid rotation direction";
      return std::nullopt;
    }
  };

  /**
   * @brief Aerodynamic properties of the drone frame
   */
  struct AerodynamicProperties {
    float dragCoeffX;           ///< Parasitic drag in body x axis N/(m/s)^2
    float dragCoeffY;           ///< Parasitic drag in body y axis N/(m/s)^2
    float dragCoeffZ;           ///< Parasitic drag in body z axis N/(m/s)^2
    bool enableAerodynamics;    ///< Enable/disable aerodynamic effects

    /**
     * @brief Gets the drag matrix
     * @return 3x3 diagonal matrix of drag coefficients
     */
    [[nodiscard]] glm::mat3 getDragMatrix() const {
      return glm::mat3(
          dragCoeffX, 0.0f, 0.0f,
          0.0f, dragCoeffY, 0.0f,
          0.0f, 0.0f, dragCoeffZ
      );
    }
  };

  /**
   * @brief Motor dynamics and control properties
   */
  struct MotorProperties {
    float responseTime;         ///< Motor response time (s)
    float noiseStdDev;         ///< Standard deviation of motor noise (rad/s)

    // Control gains
    float bodyRateGain;        ///< P gain for body rate control
    float velocityGain;        ///< P gain for velocity control
    float attitudePGain;       ///< P gain for attitude control
    float attitudeDGain;       ///< D gain for attitude control
  };

  /**
   * @brief Control input structure for different control modes
   */
  struct ControlInput {
    ControlMode mode;                      ///< Current control mode
    std::vector<float> motorSpeeds;        ///< Direct motor speeds (MOTOR_SPEEDS)
    std::vector<float> motorThrusts;       ///< Individual motor thrusts (MOTOR_THRUSTS)
    float collectiveThrust;                ///< Collective thrust
    glm::vec3 bodyRates;                   ///< Desired body rates
    glm::vec3 bodyMoments;                ///< Desired body moments
    glm::quat targetAttitude;             ///< Target attitude quaternion
    glm::vec3 targetVelocity;             ///< Target velocity
    glm::vec3 targetAcceleration;         ///< Target acceleration

    /**
     * @brief Validates control input based on mode
     * @param numRotors Number of rotors in the system
     * @return std::nullopt if valid, error message if invalid
     */
    [[nodiscard]] std::optional<std::string> validate(size_t numRotors) const;
  };
}
