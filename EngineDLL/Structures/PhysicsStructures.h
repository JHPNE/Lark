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
    inertia_prop i;
    geom_prop g;
    aero_prop a;
    rotor_prop r;
    motor_prop m;
    control_gains c;
    lower_level_controller_prop l;
};