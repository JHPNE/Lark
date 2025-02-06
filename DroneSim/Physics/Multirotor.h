#pragma once
#include "IDrone.h"

namespace lark::drones {
  /**
   * @brief Implementation of a multirotor drone with variable number of rotors
   * @details Provides a complete implementation of the IDrone interface with
   *          comprehensive physics simulation and robust error handling.
   */
  class Multirotor final : public IDrone {
    public:
      /**
       * @brief Constructs a new Multirotor instance
       *
       * @param inertial Validated inertial properties
       * @param aero Validated aerodynamic properties
       * @param motor Validated motor properties
       * @param rotors Vector of validated rotor parameters
       * @param mode Initial control mode
       *
       * @throws std::invalid_argument if any parameters are invalid
       * @throws std::runtime_error if initialization fails
       *
       * @pre rotors.size() > 0
       * @post Object is ready for simulation
       */
      explicit Multirotor(
          const InertiaProperties& inertial,
          const AerodynamicProperties& aero,
          const MotorProperties& motor,
          const std::vector<RotorParameters>& rotors,
          ControlMode mode = ControlMode::MOTOR_SPEEDS);

      // Delete copy/move operations - drones are non-copyable resources
      Multirotor(const Multirotor&) = delete;
      Multirotor& operator=(const Multirotor&) = delete;
      Multirotor(Multirotor&&) = delete;
      Multirotor& operator=(Multirotor&&) = delete;

      DroneState step(const DroneState& state, const ControlInput& control, float timeStep) override;

      std::pair<glm::vec3, glm::vec3> computeStateDerivatives(const DroneState& state, const ControlInput& control, float timeStep) override;

      [[nodiscard]] ControlMode getControlMode() const override { return controlMode; }
      void setControlMode(ControlMode mode) override;
      [[nodiscard]] size_t getRotorCount() const override { return rotors.size(); }

      [[nodiscard]] std::optional<std::string> validateState(
          const DroneState& state) const override;
      [[nodiscard]] std::optional<std::string> validateControl(
          const ControlInput& control) const override;

    private:
      // Physical properties (immutable after construction)
      const InertiaProperties inertialProps;
      const AerodynamicProperties aeroProps;
      const MotorProperties motorProps;
      const std::vector<RotorParameters> rotors;

      // cached invers inertia matrix for perf
      const glm::mat3 inverseInertia;

      // Control State
      ControlMode controlMode;

      // Allocation matrices
      glm::mat4 thrustMomentToForce; // Maps [thrust, moments] to rotor forces
      glm::mat4 forceToThrustMoment; // maps rotor forces to thrust and moments

    /**
     * @brief Computes total wrench (force and moment) in body frame
     *
     * @param bodyRates Current body angular rates
     * @param rotorSpeeds Current rotor speeds
     * @param bodyAirspeed Airspeed vector in body frame
     *
     * @return Pair of force (N) and moment (N⋅m) vectors in body frame
     * @throws std::runtime_error if computation fails
     */
      [[nodiscard]] std::pair<glm::vec3, glm::vec3> computeBodyWrench(
          const glm::vec3& bodyRates,
          const std::vector<float>& rotorSpeeds,
          const glm::vec3& bodyAirspeed) const;

    /**
     * @brief Computes commanded motor speeds based on control mode
     * @implements IDrone::computeCommandedMotorSpeeds
     */
      std::vector<float> computeCommandedMotorSpeeds(
          const DroneState& state,
          const ControlInput& control) override;

    /**
     * @brief Initializes control allocation matrices
     * @throws std::runtime_error if matrix computation fails
     */
      void initializeControlAllocation();

    /**
     * @brief Quaternion derivative computation
     *
     * @param quat Current quaternion [x,y,z,w]
     * @param omega Angular velocity vector
     * @return Quaternion derivative
     */
      [[nodiscard]] static glm::quat computeQuaternionDerivative(
           const glm::quat& quat,
           const glm::vec3& omega);

    /**
     * @brief Creates skew-symmetric matrix from vector
     * @param v Input vector
     * @return 3x3 skew-symmetric matrix
     */
      [[nodiscard]] static glm::mat3 hatMap(const glm::vec3& v);
  };
}