#include "DroneTypes.h"
#include <optional>
#include <stdexcept>

namespace lark::drones {
  /**
   * @brief Error codes specific to drone operations
   */
  enum class DroneError {
    SUCCESS = 0,
    INVALID_STATE = 1,
    INVALID_CONTROL = 2,
    INTEGRATION_FAILURE = 3,
    ALLOCATION_FAILURE = 4,
    SYSTEM_FAILURE = 5
  };
  /**
  * @brief Abstract interface for all drone implementations
  */
  class IDrone {
    public:
      virtual ~IDrone() = default;
      /**
      * @brief Steps the simulation forward by the specified time step
      *
      * @param state Current validated drone state
      * @param control Validated control input for the step
      * @param timeStep Time step in seconds (must be positive)
      *
      * @return Updated drone state
      * @throws std::invalid_argument if inputs fail validation
      * @throws std::runtime_error if simulation step fails
      *
      * @pre timeStep > 0
      * @post Returned state is valid according to validateState()
      */
      virtual DroneState step(
        const DroneState& state,
        const ControlInput& control,
        float timeStep) = 0;

      /**
      * @brief Computes state derivatives for sensor simulation
      *
      * @param state Current validated drone state
      * @param control Validated control input
      * @param timeStep Time step in seconds (must be positive)
      *
      * @return Pair of linear acceleration (m/s^2) and angular acceleration (rad/s^2)
      * @throws std::invalid_argument if inputs fail validation
      *
      * @pre timeStep > 0
      */
      virtual std::pair<glm::vec3, glm::vec3> computeStateDerivatives(
        const DroneState& state,
        const ControlInput& control,
        float timeStep) = 0;

    /**
     * @brief Gets the current control mode
     * @return Current control mode enumeration
     */
    [[nodiscard]] virtual ControlMode getControlMode() const = 0;

    /**
     * @brief Sets the control mode
     *
     * @param mode Desired control mode
     * @throws std::invalid_argument if mode is not supported
     *
     * @post getControlMode() returns the new mode
     */
    virtual void setControlMode(ControlMode mode) = 0;

    /**
     * @brief Gets the number of rotors
     * @return Number of rotors in the drone configuration
     */
    [[nodiscard]] virtual size_t getRotorCount() const = 0;

    /**
     * @brief Validates if a given state is physically possible
     *
     * @param state State to validate
     * @return std::nullopt if valid, error message if invalid
     */
    [[nodiscard]] virtual std::optional<std::string> validateState(
        const DroneState& state) const = 0;

    /**
     * @brief Validates if control input is valid for current mode
     *
     * @param control Control input to validate
     * @return std::nullopt if valid, error message if invalid
     */
    [[nodiscard]] virtual std::optional<std::string> validateControl(
        const ControlInput& control) const = 0;
  protected:
    /**
     * @brief Computes commanded motor speeds based on control mode
     *
     * @param state Current validated state
     * @param control Validated control input
     *
     * @return Vector of commanded motor speeds (rad/s)
     * @throws std::runtime_error if computation fails
     *
     * @pre state is valid according to validateState()
     * @pre control is valid according to validateControl()
     */
    virtual std::vector<float> computeCommandedMotorSpeeds(
        const DroneState& state,
        const ControlInput& control) = 0;

  };
}