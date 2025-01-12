#include "Rotor.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace lark::rotor {
    namespace {
        constexpr float EARTH_GRAVITY = -9.81f;
         // Atmospheric constants
        constexpr float SEA_LEVEL_DENSITY = 1.225f;       // kg/m^3
        constexpr float ATMOSPHERIC_SCALE_HEIGHT = 7400.f; // meters
        constexpr float SPEED_OF_SOUND = 340.29f;         // m/s at sea level
        constexpr float AIR_VISCOSITY = 1.81e-5f;         // kg/(m·s) at sea level

        // Rotor aerodynamic constants
        constexpr float DRAG_COEFFICIENT = 0.3f;          // Typical for rotor body
        constexpr float PROFILE_DRAG_COEFFICIENT = 0.015f; // Base profile drag
        constexpr float LIFT_SLOPE = 6.2f;                // Lift curve slope (2π theoretical)
        constexpr int NUM_BLADE_ELEMENTS = 10;            // Number of elements for BEM
        constexpr float MAX_RPM = 15000.0f;              // Maximum safe RPM

        // Physical limits and safety factors
        constexpr float SMALL_ROTOR_DENSITY_SCALE = 100.0f; // Scale factor for small rotor density effects
        constexpr float SAFETY_MARGIN = 0.95f;             // 5% safety margin
        constexpr float TIP_LOSS_BASE = 0.9f;              // Base tip loss factor
        constexpr float TIP_LOSS_BLADE_FACTOR = 0.0216f;   // Per-blade tip loss adjustment

        struct rotor_runtime_state {
            float current_thrust{0.0f};       // Current thrust generated (N)
            float air_density_ratio{1.0f};    // Current air density relative to sea level
        };

        // Component data extension and pool
        struct rotor_data : drone_components::component_data<drone_data::RotorBody> {
            // Configuration parameters (set once)
            float density_altitude{0.0f};     // Initial altitude for calculations
            float temperature{288.15f};       // Temperature in Kelvin (15°C default)

            // Runtime state
            rotor_runtime_state state;

            // Cached calculations (can be recomputed as needed)
            float get_hover_thrust() const { return mass * EARTH_GRAVITY; }
            float get_max_thrust() const { return get_hover_thrust() * 2.5f; }
        };

        drone_components::component_pool<rotor_id, rotor_data> pool;

        struct BladeElement {
            float radius;      // Local radius (m)
            float chord;       // Local chord length (m)
            float twist;       // Local geometric twist (rad)
            float dr;         // Element width (m)
            float local_vel;  // Local velocity (m/s)
            float local_aoa;  // Local angle of attack (rad)
        };

        float calculate_air_density_ratio(float altitude, float temperature) {
            // Combined density model for small-scale rotors
            float standard_ratio = std::exp(-altitude / ATMOSPHERIC_SCALE_HEIGHT);
            float small_scale_correction = std::exp(-altitude / SMALL_ROTOR_DENSITY_SCALE);
            return standard_ratio * small_scale_correction;
        }

        float calculate_reynolds_number(float velocity, float chord, float density) {
            return (density * velocity * chord) / AIR_VISCOSITY;
        }

        std::pair<float, float> calculate_aerodynamic_coefficients(float alpha, float reynolds) {
            const float alpha_deg = glm::degrees(alpha);
            const float stall_angle = 12.0f; // degrees

            // Lift coefficient with Reynolds number effects
            float cl = LIFT_SLOPE * std::sin(alpha);
            float re_factor = std::min(1.0f, std::max(0.7f, reynolds / 70000.0f));
            cl *= re_factor;

            // Stall behavior
            if (std::abs(alpha_deg) > stall_angle) {
                float post_stall = 0.9f * (1.0f - std::min(1.0f, (std::abs(alpha_deg) - stall_angle) / 15.0f));
                cl *= post_stall;
            }

            // Profile drag
            float cd = PROFILE_DRAG_COEFFICIENT * (1.0f + std::abs(alpha));
            cd += 0.01f * (1.0f - re_factor);

            return {cl, cd};
        }

        float calculate_tip_loss(const rotor_data* data) {
            return TIP_LOSS_BASE - TIP_LOSS_BLADE_FACTOR * data->bladeCount;
        }

        std::vector<BladeElement> initialize_blade_elements(const rotor_data* data) {
            std::vector<BladeElement> elements(NUM_BLADE_ELEMENTS);
            const float dr = data->bladeRadius / NUM_BLADE_ELEMENTS;
            const float root_chord = 0.15f * data->bladeRadius;
            const float tip_chord = 0.08f * data->bladeRadius;
            const float root_twist = data->bladePitch + 0.175f;
            const float tip_twist = data->bladePitch;

            for (int i = 0; i < NUM_BLADE_ELEMENTS; i++) {
                float r_ratio = float(i + 0.5f) / NUM_BLADE_ELEMENTS;
                elements[i].radius = r_ratio * data->bladeRadius;
                elements[i].dr = dr;
                elements[i].chord = root_chord + (tip_chord - root_chord) * r_ratio;
                elements[i].twist = root_twist + (tip_twist - root_twist) * r_ratio;
            }

            return elements;
        }

        void update_element_conditions(BladeElement& element,
                                 const rotor_data* data,
                                 float omega,
                                 const btVector3& local_velocity) {
            float Ut = omega * element.radius;
            float Up = local_velocity.dot(data->rotorNormal);

            float thrust_coeff = data->state.current_thrust /
                               (data->airDensity * data->state.air_density_ratio * data->discArea *
                                std::pow(omega * data->bladeRadius, 2));
            float induced_vel = std::sqrt(std::max(0.0f, thrust_coeff / 2.0f));
            Up += induced_vel;

            float phi = std::atan2(Up, Ut);
            element.local_aoa = element.twist - phi;
            element.local_vel = std::sqrt(Ut * Ut + Up * Up);
        }

        std::pair<float, float> calculate_element_forces(const BladeElement& element,
                                                       const rotor_data* data,
                                                       float density) {
            float reynolds = calculate_reynolds_number(element.local_vel, element.chord, density);
            auto [cl, cd] = calculate_aerodynamic_coefficients(element.local_aoa, reynolds);

            float q = 0.5f * density * element.local_vel * element.local_vel;
            float dL = cl * q * element.chord * element.dr;
            float dD = cd * q * element.chord * element.dr;

            float phi = std::atan2(dD, dL);
            float dN = dL * std::cos(phi) + dD * std::sin(phi);
            float dT = dL * std::sin(phi) - dD * std::cos(phi);

            return {dN, dT};
        }

        std::pair<float, float> calculate_bem_forces(const rotor_data* data,
                                               const btVector3& local_velocity) {
            if (!data || data->currentRPM <= 0.0f) return {0.0f, 0.0f};

            const float omega = (data->currentRPM * 2.0f * glm::pi<float>()) / 60.0f;
            const float density = data->airDensity * data->state.air_density_ratio;

            auto elements = initialize_blade_elements(data);
            float total_thrust = 0.0f;
            float total_torque = 0.0f;

            for (auto& element : elements) {
                update_element_conditions(element, data, omega, local_velocity);
                auto [dN, dT] = calculate_element_forces(element, data, density);

                float mach_number = (omega * element.radius) / SPEED_OF_SOUND;
                float mach_correction = mach_number > 0.3f ?
                    std::min(1.2f, 1.0f / std::sqrt(1.0f - std::min(0.64f, mach_number * mach_number))) :
                    1.0f;

                dN *= mach_correction * data->bladeCount;
                dT *= data->bladeCount;

                total_thrust += dN;
                total_torque += dT * element.radius;
            }

            float tip_loss = calculate_tip_loss(data);
            return {total_thrust * tip_loss, total_torque * tip_loss};
        }
    }

    void drone_component::calculate_forces(float deltaTime) {
        auto* data = pool.get_data(get_id());
        if (!data || !data->rigidBody || !data->is_valid) return;

        btTransform trans;
        data->rigidBody->getMotionState()->getWorldTransform(trans);
        data->position = trans.getOrigin();

        float alt_pos = data->position.y();
        float altitude = std::max(0.0f, alt_pos);
        data->state.air_density_ratio = calculate_air_density_ratio(altitude, data->temperature);

        btVector3 linear_velocity = data->rigidBody->getLinearVelocity();
        btVector3 angular_velocity = data->rigidBody->getAngularVelocity();

        float velocity_magnitude = linear_velocity.length();
        float drag_force_magnitude = 0.5f * data->airDensity * data->state.air_density_ratio *
                                   DRAG_COEFFICIENT * data->discArea *
                                   velocity_magnitude * velocity_magnitude;

        btVector3 drag_force = velocity_magnitude > 0.001f ?
            -drag_force_magnitude * linear_velocity.normalized() : btVector3(0, 0, 0);

        auto [thrust, torque] = calculate_bem_forces(data, linear_velocity);
        data->state.current_thrust = thrust;

        btVector3 thrust_force = data->rotorNormal * thrust;
        data->rigidBody->applyCentralForce(thrust_force);
        data->rigidBody->applyCentralForce(drag_force);

        float induced_velocity = std::sqrt(
            thrust / (2.0f * data->airDensity * data->state.air_density_ratio * data->discArea)
        );
        float induced_power = thrust * induced_velocity;

        btVector3 torque_vector = -data->rotorNormal * torque;
        data->rigidBody->applyTorque(torque_vector);

        const float omega = (data->currentRPM * 2.0f * glm::pi<float>()) / 60.0f;
        float profile_power = std::abs(torque * omega);
        data->powerConsumption = profile_power + induced_power;

        float altitude_factor = std::max(0.2f, std::min(1.0f, data->state.air_density_ratio));
        data->rigidBody->setDamping(0.2f * altitude_factor, 0.7f * altitude_factor);
    }

    void drone_component::initialize() {
        auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return;

        data->state.air_density_ratio = calculate_air_density_ratio(
            data->density_altitude,
            data->temperature
        );
    }

    void drone_component::set_rpm(float target_rpm) {
        auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return;
        data->currentRPM = std::clamp(target_rpm, 0.0f, MAX_RPM);
    }

    float drone_component::get_thrust() const {
        const auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return 0.0f;
        return data->state.current_thrust;
    }

    float drone_component::get_power_consumption() const {
        const auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return 0.0f;
        return data->powerConsumption;
    }

    float drone_component::estimate_equilibrium_height(float target_rpm) const {
        const auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return 0.0f;

        const float omega = (target_rpm * 2.0f * glm::pi<float>()) / 60.0f;
        float tip_speed = omega * data->bladeRadius;

        // Calculate base thrust at sea level
        float base_thrust = 0.5f * SEA_LEVEL_DENSITY * std::pow(tip_speed, 2) *
                          data->discArea * data->liftCoefficient *
                          std::pow(std::sin(data->bladePitch), 2);

        // Apply Mach effects
        float mach_number = tip_speed / SPEED_OF_SOUND;
        if (mach_number > 0.3f) {
            float mach_correction = std::min(1.2f,
                1.0f / std::sqrt(1.0f - std::min(0.64f, mach_number * mach_number)));
            base_thrust *= mach_correction;
        }

        // Required thrust for hover
        const float weight_force = data->mass * EARTH_GRAVITY;
        if (base_thrust <= weight_force) return 0.0f;

        // Calculate equilibrium height
        float height = -SMALL_ROTOR_DENSITY_SCALE * std::log(weight_force / base_thrust);
        return std::min(height * SAFETY_MARGIN, 100.0f); // Cap at 100m for small rotors
    }

    float drone_component::get_max_theoretical_height(float target_rpm) const {
        const auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return 0.0f;

        return estimate_equilibrium_height(target_rpm) * 1.5f; // 50% more than equilibrium
    }

    drone_component create(init_info info, drone_entity::entity entity) {
        return drone_component{ pool.create(info, entity) };
    }

    void remove(drone_component c) {
        pool.remove(c.get_id());
    }
};