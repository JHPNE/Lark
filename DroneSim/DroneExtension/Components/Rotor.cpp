#include "Rotor.h"
#include <algorithm>
#include <cmath>

namespace lark::rotor {
    namespace {
        // Physical constants
        constexpr float EARTH_GRAVITY = -9.81f;
        constexpr float AIR_DENSITY_SEA_LEVEL = 1.225f;
        constexpr float SPEED_OF_SOUND = 340.29f;
        constexpr float EPSILON = 1e-6f;
        constexpr float KINEMATIC_VISCOSITY = 1.46e-5f;
        constexpr float SPECIFIC_GAS_CONSTANT = 287.058f;
        constexpr float TEMPERATURE_LAPSE_RATE = 0.0065f;
        constexpr float SEA_LEVEL_TEMPERATURE = 288.15f;

        // Aerodynamic constants for NACA 0012 airfoil
        constexpr float CRITICAL_RE = 70000.0f;
        constexpr int NUM_BLADE_ELEMENTS = 20;
        constexpr float LIFT_SLOPE = 5.73f;  // Adjusted from 2π to match real propeller data
        constexpr float MAX_CL = 1.2f;       // Reduced maximum lift coefficient
        constexpr float MIN_CD = 0.015f;     // Increased minimum drag
        constexpr float ASPECT_RATIO = 6.0f; // Typical for drone propellers

        struct AerodynamicState {
            float thrust{0.0f};
            float torque{0.0f};
            float induced_velocity{0.0f};
            float power{0.0f};
            float reynolds_number{0.0f};
            float mach_number{0.0f};
        };

        using rotor_data = drone_components::component_data<drone_data::RotorBody>;
        drone_components::component_pool<rotor_id, rotor_data> pool;

        float calculate_temperature(float altitude) {
            return SEA_LEVEL_TEMPERATURE - TEMPERATURE_LAPSE_RATE * altitude;
        }

        float calculate_air_density(float altitude) {
            float temperature = calculate_temperature(altitude);
            float pressure_ratio = std::pow(temperature / SEA_LEVEL_TEMPERATURE, 5.2561f);
            return AIR_DENSITY_SEA_LEVEL * pressure_ratio;
        }

        float calculate_dynamic_viscosity(float temperature) {
            constexpr float SUTHERLAND_CONSTANT = 110.4f;
            constexpr float REF_TEMP = 273.15f;
            constexpr float REF_VISCOSITY = 1.716e-5f;

            return REF_VISCOSITY * std::pow(temperature / REF_TEMP, 1.5f) *
                   (REF_TEMP + SUTHERLAND_CONSTANT) / (temperature + SUTHERLAND_CONSTANT);
        }

        float calculate_reynolds_number(float velocity, float chord, float air_density, float temperature) {
            float dynamic_viscosity = calculate_dynamic_viscosity(temperature);
            return (air_density * velocity * chord) / dynamic_viscosity;
        }

        btVector3 safe_normalize(const btVector3& v) {
            float length = v.length();
            return length > EPSILON ? v / length : btVector3(0, 1, 0);
        }

        float calculate_lift_coefficient(float alpha, float mach, float reynolds) {
            // Prandtl-Glauert compressibility correction
            float beta = std::sqrt(1.0f - std::min(0.95f, mach * mach));
            float corrected_lift_slope = LIFT_SLOPE / beta;

            // Reynolds number correction
            float re_factor = std::min(1.0f, std::max(0.6f, reynolds / CRITICAL_RE));
            corrected_lift_slope *= re_factor;

            // More realistic stall behavior
            constexpr float stall_angle = 12.0f * glm::pi<float>() / 180.0f;  // Reduced stall angle

            if (std::abs(alpha) <= stall_angle) {
                float cl = corrected_lift_slope * std::sin(alpha) * std::cos(alpha);
                return std::min(MAX_CL, cl);
            }

            // Post-stall behavior
            float stall_cl = corrected_lift_slope * std::sin(stall_angle) * std::cos(stall_angle);
            float post_stall = stall_cl * (std::cos(2.0f * (alpha - stall_angle)) + 1.0f) * 0.5f;
            return std::copysign(std::min(std::abs(post_stall), MAX_CL), post_stall);
        }

        float calculate_drag_coefficient(float cl, float mach, float reynolds) {
            // Profile drag with Reynolds number dependency
            float cd_profile = MIN_CD * std::pow(CRITICAL_RE / reynolds, 0.2f);

            // Induced drag with better efficiency factor
            float oswald_efficiency = 0.85f;  // Typical for propellers
            float induced_drag = (cl * cl) / (glm::pi<float>() * ASPECT_RATIO * oswald_efficiency);

            // Wave drag
            float cd_wave = 0.0f;
            if (mach > 0.7f) {
                cd_wave = 20.0f * std::pow(mach - 0.7f, 4);
            }

            return cd_profile + induced_drag + cd_wave;
        }

        void calculate_blade_forces(rotor_data* data, AerodynamicState& state, const btVector3& velocity) {
            float current_alt = data->position.y();
            float altitude = std::max(0.0f, current_alt);
            float temperature = calculate_temperature(altitude);
            float air_density = calculate_air_density(altitude);
            float omega = (data->currentRPM * 2.0f * glm::pi<float>()) / 60.0f;

            btVector3 normal = safe_normalize(data->rotorNormal);
            float axial_velocity = velocity.dot(normal);

            float dr = data->bladeRadius / NUM_BLADE_ELEMENTS;
            float total_thrust = 0.0f;
            state.torque = 0.0f;

            // Modified blade geometry for more realistic propeller shape
            float chord_root = 0.12f * data->bladeRadius;  // Reduced chord
            float chord_tip = 0.04f * data->bladeRadius;   // Thinner tip
            float pitch_root = data->bladePitch * 1.2f;    // Increased root pitch
            float pitch_tip = data->bladePitch * 0.8f;     // Reduced tip pitch

            // Initial induced velocity estimate
            float disk_loading = (data->mass * std::abs(EARTH_GRAVITY)) / data->discArea;
            state.induced_velocity = std::sqrt(disk_loading / (2.0f * air_density));

            // BEMT iteration
            for (int iteration = 0; iteration < 10; ++iteration) {
                float prev_induced = state.induced_velocity;
                total_thrust = 0.0f;

                for (int i = 0; i < NUM_BLADE_ELEMENTS; ++i) {
                    float r = (i + 0.5f) * dr;
                    float radius_ratio = r / data->bladeRadius;

                    // Non-linear chord and twist distribution
                    float chord = chord_root + (chord_tip - chord_root) * std::pow(radius_ratio, 0.8f);
                    float twist = pitch_root + (pitch_tip - pitch_root) * radius_ratio;

                    float tangential_speed = omega * r;
                    float local_velocity = std::sqrt(
                        std::pow(state.induced_velocity - axial_velocity, 2) +
                        std::pow(tangential_speed, 2));

                    state.reynolds_number = calculate_reynolds_number(
                        local_velocity, chord, air_density, temperature);
                    state.mach_number = local_velocity / SPEED_OF_SOUND;

                    float phi = std::atan2(state.induced_velocity - axial_velocity, tangential_speed);
                    float alpha = twist - phi;

                    float cl = calculate_lift_coefficient(alpha, state.mach_number, state.reynolds_number);
                    float cd = calculate_drag_coefficient(cl, state.mach_number, state.reynolds_number);

                    // Improved Prandtl tip loss model
                    float B = data->bladeCount * (1.0f - radius_ratio) /
                             (2.0f * radius_ratio * std::sin(std::max(EPSILON, std::abs(phi))));
                    float F = (2.0f / glm::pi<float>()) * std::acos(std::exp(-B));

                    float dynamic_pressure = 0.5f * air_density * local_velocity * local_velocity;
                    float dA = chord * dr;

                    // Force components with improved tip loss model
                    float dL = dynamic_pressure * dA * cl * F;
                    float dD = dynamic_pressure * dA * cd * F;

                    float dT = data->bladeCount * (dL * std::cos(phi) - dD * std::sin(phi));
                    float dQ = data->bladeCount * r * (dL * std::sin(phi) + dD * std::cos(phi));

                    total_thrust += dT;
                    state.torque += dQ;
                }

                // Update induced velocity with improved wake model
                if (total_thrust > 0.0f) {
                    float wake_contraction = 0.95f;  // Wake contraction factor
                    state.induced_velocity = wake_contraction * std::sqrt(total_thrust /
                        (2.0f * air_density * data->discArea *
                         std::sqrt(1.0f + std::pow(axial_velocity/std::max(EPSILON, state.induced_velocity), 2))));
                }

                if (std::abs(state.induced_velocity - prev_induced) < 0.01f) break;
            }

            state.thrust = total_thrust;
            state.power = std::abs(state.torque * omega);
            data->powerConsumption = state.power;
        }
    }

    void drone_component::calculate_forces(float deltaTime) {
        auto* data = pool.get_data(get_id());
        if (!data || !data->rigidBody || !data->is_valid) return;

        btTransform trans;
        data->rigidBody->getMotionState()->getWorldTransform(trans);
        data->position = trans.getOrigin();

        static AerodynamicState state;
        btVector3 velocity = data->rigidBody->getLinearVelocity();

        calculate_blade_forces(data, state, velocity);

        btVector3 thrust_force = safe_normalize(data->rotorNormal) * state.thrust;

        // Parasitic drag
        float air_density = calculate_air_density(data->position.y());
        float parasitic_drag_coeff = 0.1f;
        btVector3 drag_force = -velocity * velocity.length() *
                              (0.5f * air_density * parasitic_drag_coeff * data->discArea);

        data->rigidBody->applyCentralForce(thrust_force + drag_force);
        data->rigidBody->applyTorque(-data->rotorNormal * state.torque);

        // Natural aerodynamic damping
        float damping = 0.1f * std::exp(-data->position.y() / 7400.0f);
        data->rigidBody->setDamping(damping, damping);
    }

    void drone_component::initialize() {
        auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return;

        data->discArea = std::max(EPSILON,
            glm::pi<float>() * data->bladeRadius * data->bladeRadius);
        data->powerConsumption = 0.0f;
        data->currentRPM = 5000.0f;

        if (data->rotorNormal.length2() < EPSILON) {
            data->rotorNormal = btVector3(0, 1, 0);
        } else {
            data->rotorNormal = safe_normalize(data->rotorNormal);
        }

        if (data->rigidBody) {
            data->rigidBody->setDamping(0.1f, 0.1f);
        }
    }

    void drone_component::set_rpm(float target_rpm) {
        auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return;
        data->currentRPM = std::max(0.0f, target_rpm);
    }

    float drone_component::get_thrust() const {
        const auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return 0.0f;

        static AerodynamicState state;
        btVector3 zero_velocity(0, 0, 0);

        rotor_data temp_data = *data;
        calculate_blade_forces(&temp_data, state, zero_velocity);

        return state.thrust;
    }

    float drone_component::get_power_consumption() const {
        const auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return 0.0f;
        return data->powerConsumption;
    }

    float drone_component::estimate_equilibrium_height(float target_rpm) const {
        const auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid || target_rpm <= 0.0f) return 0.0f;

        rotor_data temp_data = *data;
        temp_data.currentRPM = target_rpm;

        static AerodynamicState state;
        btVector3 zero_velocity(0, 0, 0);

        // Calculate sea-level thrust
        temp_data.position.setY(0.0f);
        calculate_blade_forces(&temp_data, state, zero_velocity);
        float sea_level_thrust = state.thrust;
        float weight = data->mass * std::abs(EARTH_GRAVITY);

        if (sea_level_thrust <= weight) return 0.0f;

        // Binary search for equilibrium height
        float min_height = 0.0f;
        float max_height = 20000.0f;
        const float tolerance = 0.1f;

        for (int i = 0; i < 50 && (max_height - min_height) > tolerance; ++i) {
            float mid_height = (min_height + max_height) * 0.5f;
            temp_data.position.setY(mid_height);

            calculate_blade_forces(&temp_data, state, zero_velocity);
            float thrust_at_height = state.thrust;

            if (thrust_at_height > weight) {
                min_height = mid_height;
            } else {
                max_height = mid_height;
            }
        }

        return min_height;
    }

    float drone_component::get_max_theoretical_height(float target_rpm) const {
        const auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid || target_rpm <= 0.0f) return 0.0f;

        // Calculate maximum height where thrust equals weight
        return estimate_equilibrium_height(target_rpm) * 1.1f;
    }

    drone_component create(init_info info, drone_entity::entity entity) {
        return drone_component{ pool.create(info, entity) };
    }

    void remove(drone_component c) {
        pool.remove(c.get_id());
    }
};;;