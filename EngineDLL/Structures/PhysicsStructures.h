#pragma once

struct inertia_prop
{
    float mass;
    glm::vec3 principal_inertia;
    glm::vec3 product_inertia;
};

struct geom_prop
{
    static constexpr size_t num_rotors = 4;
    float rotor_radius;
    std::array<glm::vec3, num_rotors> rotor_positions;
    glm::vec4 rotor_directions;

    // currently not implemented so w/e
    glm::vec3 imu_positions;
};

struct aero_prop
{
    glm::vec3 parasitic_drag;
};

struct rotor_prop
{
    float k_eta;
    float k_m;
    float k_d;
    float k_z;
    float k_h;
    float k_flap;
};

struct motor_prop
{
    float tau_m;
    float rotor_speed_min;
    float rotor_speed_max;
    float motor_noise_std;
};

struct control_gains
{
    glm::vec3 kp_pos{6.5f, 6.5f, 15.0f};
    glm::vec3 kd_pos{4.0f, 4.0f, 9.0f};
    float kp_att = 544.0f;
    float kd_att = 46.64f;
    glm::vec3 kp_vel{0.65f, 0.65f, 1.5f};
};

struct lower_level_controller_prop
{
    float k_w;
    float k_v;
    int kp_att;
    float kd_att;
};

struct quad_params
{
    inertia_prop i{};
    geom_prop g{};
    aero_prop a{};
    rotor_prop r{};
    motor_prop m{};
    control_gains c;
    lower_level_controller_prop l{};
};

struct drone_state
{
    glm::vec3 position;
    glm::vec3 velocity;
    glm::vec4 attitude;
    glm::vec3 body_rates;
    glm::vec3 wind;
    glm::vec4 rotor_speeds;
};

enum class control_abstraction
{
    /// @brief Direct motor speed control (rad/s)
    CMD_MOTOR_SPEEDS,

    /// @brief Individual rotor thrust commands (N)
    CMD_MOTOR_THRUSTS,

    /// @brief Collective thrust (N) + body angular rates (rad/s)
    CMD_CTBR,

    /// @brief Collective thrust (N) + body moments (N⋅m)
    CMD_CTBM,

    /// @brief Collective thrust (N) + attitude quaternion
    CMD_CTATT,

    /// @brief Velocity vector in world frame (m/s)
    CMD_VEL,

    /// @brief Acceleration vector in world frame (m/s²)
    CMD_ACC
};

struct control_input
{
    // Motor level commands
    glm::vec4 cmd_motor_speeds;  // rad/s - for CMD_MOTOR_SPEEDS
    glm::vec4 cmd_motor_thrusts; // N - for CMD_MOTOR_THRUSTS

    // Force and moment commands
    float cmd_thrust;    // N - collective thrust for CMD_CTBR, CMD_CTBM, CMD_CTATT
    glm::vec3 cmd_moment; // N⋅m - for CMD_CTBM

    // Attitude commands
    glm::vec4 cmd_q; // quaternion [x,y,z,w] - for CMD_CTATT
    glm::vec3 cmd_w; // rad/s - body rates for CMD_CTBR

    // High-level commands
    glm::vec3 cmd_v;   // m/s - velocity in world frame for CMD_VEL
    glm::vec3 cmd_acc; // m/s² - acceleration in world frame for CMD_ACC
};

enum class trajectory_type
{
    Circular,
    Chaos
};

struct trajectory
{
    trajectory_type type;
    glm::vec3 position{0.f,0.f,0.f};
    float delta{1.0f};
    float radius{1.0f};
    float frequency{0.5f};
    int n_points{10};
    float segment_time{1.0f};
};

enum class wind_type
{
    NoWind,
    ConstantWind,
    SinusoidWind,
    LadderWind
};

struct wind
{
    wind_type type;
    glm::vec3 w;

    // Additional parameters for different wind types
    // SinusoidWind parameters
    glm::vec3 amplitudes{1.0f, 1.0f, 1.0f};
    glm::vec3 frequencies{1.0f, 1.0f, 1.0f};
    glm::vec3 phase{0.0f, 0.0f, 0.0f};

    // LadderWind parameters
    glm::vec3 min{-1.0f, -1.0f, -1.0f};
    glm::vec3 max{1.0f, 1.0f, 1.0f};
    glm::vec3 duration{1.0f, 1.0f, 1.0f};
    glm::vec3 n_steps{5.0f, 5.0f, 5.0f};
    bool random{false};
};



