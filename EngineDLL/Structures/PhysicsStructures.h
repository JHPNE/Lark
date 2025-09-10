#pragma once

struct physics_inertia_properties
{
    float mass;
    float Ixx;
    float Iyy;
    float Izz;
    float Ixy;
    float Iyz;
    float Ixz;
};

struct physics_aerodynamic_properties
{
    float dragCoeffX;
    float dragCoeffY;
    float dragCoeffZ;
    bool enableAerodynamics;
};

struct physics_motor_properties
{
    float responseTime;
    float noiseStdDev;
    float bodyRateGain;
    float velocityGain;
    float attitudePGain;
    float attitudeDGain;
};

struct physics_rotor_parameters
{
    float thrustCoeff;
    float torqueCoeff;
    float dragCoeff;
    float inflowCoeff;
    float flapCoeff;
    glm::vec3 position;
    int direction;
    float minSpeed;
    float maxSpeed;
};

enum class PhysicsControlMode : uint8_t
{
    MOTOR_SPEEDS,
    MOTOR_THRUSTS,
    COLLECTIVE_THRUST_BODY_RATES,
    COLLECTIVE_THRUST_BODY_MOMENTS,
    COLLECTIVE_THRUST_ATTITUDE,
    VELOCITY,
    ACCELERATION
};

struct physics_control_input
{
    PhysicsControlMode mode;
    std::vector<float> motorSpeeds;
    std::vector<float> motorThrusts;
    float collectiveThrust;
    glm::vec3 bodyRates;
    glm::vec3 bodyMoments;
    glm::quat targetAttitude;
    glm::vec3 targetVelocity;
    glm::vec3 targetAcceleration;
};

struct physics_drone_state
{
    glm::vec3 position;
    glm::vec3 velocity;
    glm::quat orientation;
    glm::vec3 angular_velocity;
    glm::vec3 wind;
    std::vector<float> rotor_speeds;
};

enum class WindType
{
    NO_WIND,
    CONSTANT_WIND,
    DRYDEN_GUST
};

struct physics_wind_params
{
    WindType type;
    glm::vec3 constant_wind_velocity; // For CONSTANT_WIND

    // For DRYDEN_GUST
    glm::vec3 mean_wind;
    float altitude;
    float wingspan;
    float turbulence_level;
};
