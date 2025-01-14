#include "Rotor.h"
#include <algorithm>
#include <cmath>

namespace lark::rotor {
    namespace {
        constexpr float PI = glm::pi<float>();
        constexpr float RAD_TO_RPM = 60 / (2 * PI);
        constexpr float RPM_TO_RAD = (2 * PI) / 60;
        constexpr float SEA_LEVEL_DENSITY = 1.225f; // kg/m^3
        constexpr float SCALE_HEIGHT = 8500.0f; // meters
        constexpr float GRAVITY = 9.81f; // m/s

        using rotor_data = drone_components::component_data<drone_data::RotorBody>;
        drone_components::component_pool<rotor_id, rotor_data> pool;

        float calculate_air_density(float altitude) {
            return SEA_LEVEL_DENSITY * std::exp(-altitude / SCALE_HEIGHT);
        }

        float calculate_disc_area(float radius) {
            return PI * radius * radius;
        }

        float calculate_drag(float velocity, float air_density, float disc_area, float drag_coefficient) {
           return 0.5f * air_density * velocity * velocity * disc_area * drag_coefficient;
        }

        float calculate_tip_speed(float radius, float rpm) {
            const float angular_velocity = rpm * RPM_TO_RAD;
            return radius * angular_velocity;
        }

        float calculate_thrust(const rotor_data* data, float airdensity) {
            if (!data || !data->is_valid) return 0.0f;

            const float tip_speed = calculate_tip_speed(data->bladeRadius, data->currentRPM);

            float thrust = static_cast<float>(data->bladeCount) *
                data->liftCoefficient *
                0.5f *
                airdensity *
                data->discArea *
                tip_speed * tip_speed;

            // Apply blade interference factor - efficiency decreases with more blades
            const float interference_factor = 1.0f - (0.05f * (data->bladeCount - 1));
            thrust *= interference_factor;

            // RPM efficiency factor - diminishing returns at very high RPM
            const float rpm_factor = 1.0f - std::exp(-data->currentRPM / 15000.0f);
            thrust *= rpm_factor;

            return thrust;
        }

        float calculate_power(const rotor_data* data, const float thrust, float air_density) {
            if (!data || !data->is_valid) return 0.0f;

            const float induced_velocity = std::sqrt(thrust / (2.0f * air_density * data->discArea));
            float power = thrust * induced_velocity;

            const float profile_power = 0.1f * power; // Simplified model
            power += profile_power;

            // Account for compressibility losses at high tip speeds
            const float tip_speed = calculate_tip_speed(data->bladeRadius, data->currentRPM);
            const float mach_factor = 1.0f + std::pow(tip_speed / 340.0f, 3); // 340 m/s is speed of sound
            power *= mach_factor;

            return power;
        }

    }

    void drone_component::calculate_forces(float deltaTime) {
        auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return;

        // Get current altitude from position
        const float altitude = data->position.getY();
        const float air_density = calculate_air_density(altitude);


        const float thrust = calculate_thrust(data, air_density);

        float vertical_velocity = 0.0f;
        if (data->rigidBody) {
            vertical_velocity = data->rigidBody->getLinearVelocity().y();
        }

        const float drag = calculate_drag(vertical_velocity, air_density, data->discArea, 0.8f);

        // Calculate power consumption
        const float power = calculate_power(data, thrust, air_density);
        data->powerConsumption = power;

        // Apply forces to rigid body if available
        if (data->rigidBody) {
            // Calculate net force considering drag
            btVector3 net_force = data->rotorNormal * (thrust - drag);
            data->rigidBody->applyCentralForce(net_force);
        }
    }

    void drone_component::initialize() {
        auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return;

        data->currentRPM = 1000.0f;
        data->powerConsumption = 0.0f;
        data->discArea = calculate_disc_area(data->bladeRadius);
    }

    void drone_component::set_rpm(float target_rpm) {
        auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return;

        // Apply realistic motor response
        const float max_rpm_change = 1000.0f; // RPM per second
        const float current_rpm = data->currentRPM;
        const float rpm_diff = target_rpm - current_rpm;
        const float rpm_change = std::clamp(rpm_diff, -max_rpm_change, max_rpm_change);

        data->currentRPM = std::max(0.0f, current_rpm + rpm_change);
    }

    float drone_component::get_thrust() const {
        const auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return 0.0f;

        const float altitude = data->position.getY();
        const float air_density = calculate_air_density(altitude);
        return calculate_thrust(data, air_density);
    }

    float drone_component::get_power_consumption() const {
        const auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return 0.0f;
        return data->powerConsumption;
    }

    drone_component create(init_info info, drone_entity::entity entity) {
        info.discArea = calculate_disc_area(info.bladeRadius);
        return drone_component{ pool.create(info, entity) };
    }

    void remove(drone_component c) {
        pool.remove(c.get_id());
    }
}