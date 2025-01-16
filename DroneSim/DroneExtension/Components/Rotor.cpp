#include "Rotor.h"
#include <algorithm>
#include <cmath>

namespace lark::rotor {
    namespace {
        // Basic Constants
        constexpr float PI = glm::pi<float>();
        constexpr float RAD_TO_RPM = 60.0f / (2.0f * PI);
        constexpr float RPM_TO_RAD = (2.0f * PI) / 60.0f;

        // ISA Model Constants
        constexpr float ISA_SEA_LEVEL_PRESSURE = 101325.0f;    // Pa
        constexpr float ISA_SEA_LEVEL_TEMPERATURE = 288.15f;   // K (15°C)
        constexpr float ISA_SEA_LEVEL_DENSITY = 1.225f;        // kg/m³
        constexpr float ISA_LAPSE_RATE = -0.0065f;             // K/m (up to troposphere)
        constexpr float ISA_GAS_CONSTANT = 287.05f;            // J/(kg·K)
        constexpr float ISA_GRAVITY = 9.80665f;                // m/s²
        constexpr float ISA_TROPOPAUSE_ALTITUDE = 11000.0f;    // m
        constexpr float ISA_TROPOPAUSE_TEMPERATURE = 216.65f;  // K
        constexpr float ISA_GAMMA = 1.4f;                      // Ratio of specific heats for air

        using rotor_data = drone_components::component_data<drone_data::RotorBody>;
        drone_components::component_pool<rotor_id, rotor_data> pool;

        struct AtmosphericConditions {
            float density;      // kg/m³
            float temperature;  // K
            float pressure;     // Pa
            float viscosity;    // kg/(m·s)
            float mach_factor;  // dimensionless
            float speed_of_sound; // m/s
        };

        AtmosphericConditions calculate_atmospheric_conditions(float altitude, float velocity) {
            AtmosphericConditions conditions{};

            // Ensure non-negative altitude
            altitude = std::max(0.0f, altitude);

            // Temperature calculation based on ISA model
            if (altitude <= ISA_TROPOPAUSE_ALTITUDE) {
                conditions.temperature = ISA_SEA_LEVEL_TEMPERATURE + ISA_LAPSE_RATE * altitude;
            } else {
                conditions.temperature = ISA_TROPOPAUSE_TEMPERATURE;
            }

            // Pressure calculation
            if (altitude <= ISA_TROPOPAUSE_ALTITUDE) {
                float exponent = -ISA_GRAVITY / (ISA_GAS_CONSTANT * ISA_LAPSE_RATE);
                conditions.pressure = ISA_SEA_LEVEL_PRESSURE *
                    std::pow(conditions.temperature / ISA_SEA_LEVEL_TEMPERATURE, exponent);
            } else {
                float base_pressure = ISA_SEA_LEVEL_PRESSURE *
                    std::pow(ISA_TROPOPAUSE_TEMPERATURE / ISA_SEA_LEVEL_TEMPERATURE,
                            -ISA_GRAVITY / (ISA_GAS_CONSTANT * ISA_LAPSE_RATE));
                float exponent = -ISA_GRAVITY * (altitude - ISA_TROPOPAUSE_ALTITUDE) /
                                (ISA_GAS_CONSTANT * ISA_TROPOPAUSE_TEMPERATURE);
                conditions.pressure = base_pressure * std::exp(exponent);
            }

            // Density from ideal gas law
            conditions.density = conditions.pressure / (ISA_GAS_CONSTANT * conditions.temperature);

            // Dynamic viscosity using Sutherland's law
            constexpr float SUTHERLAND_TEMP = 273.15f;
            constexpr float SUTHERLAND_C = 120.0f;
            constexpr float SUTHERLAND_REF_VISC = 1.716e-5f;

            conditions.viscosity = SUTHERLAND_REF_VISC *
                std::pow(conditions.temperature / SUTHERLAND_TEMP, 1.5f) *
                ((SUTHERLAND_TEMP + SUTHERLAND_C) / (conditions.temperature + SUTHERLAND_C));

            // Speed of sound and Mach number
            conditions.speed_of_sound = std::sqrt(ISA_GAMMA * ISA_GAS_CONSTANT * conditions.temperature);
            conditions.mach_factor = velocity / conditions.speed_of_sound;

            return conditions;
        }

        float calculate_density_altitude_factor(const AtmosphericConditions& conditions) {
            float density_ratio = conditions.density / ISA_SEA_LEVEL_DENSITY;
            float temp_ratio = conditions.temperature / ISA_SEA_LEVEL_TEMPERATURE;
            float pressure_ratio = conditions.pressure / ISA_SEA_LEVEL_PRESSURE;

            // Apply non-linear effects of density on thrust
            float density_effect = std::pow(density_ratio, 1.5f);

            // Temperature correction based on ISA deviation
            float temp_correction = 1.0f;
            if (conditions.temperature > ISA_SEA_LEVEL_TEMPERATURE) {
                float delta_t = conditions.temperature - ISA_SEA_LEVEL_TEMPERATURE;
                temp_correction = 1.0f - (delta_t / ISA_SEA_LEVEL_TEMPERATURE) * 0.02f;
            }

            // Pressure effect using standard atmosphere relationship
            float pressure_effect = std::pow(pressure_ratio, 0.190284f);

            return density_effect * temp_correction * pressure_effect;
        }

        float calculate_altitude_efficiency(float altitude, const AtmosphericConditions& conditions) {
            // Calculate ISA standard temperature at this altitude
            float isa_temp = (altitude <= ISA_TROPOPAUSE_ALTITUDE) ?
                ISA_SEA_LEVEL_TEMPERATURE + ISA_LAPSE_RATE * altitude :
                ISA_TROPOPAUSE_TEMPERATURE;

            // Temperature deviation from ISA
            float temp_deviation = conditions.temperature - isa_temp;
            float temp_factor = std::exp(-std::abs(temp_deviation) / ISA_SEA_LEVEL_TEMPERATURE);

            // Density ratio effect
            float density_ratio = conditions.density / ISA_SEA_LEVEL_DENSITY;
            float density_effect = std::exp(-std::pow(1.0f - density_ratio, 2.0f));

            // Pressure ratio effect
            float pressure_ratio = conditions.pressure / ISA_SEA_LEVEL_PRESSURE;
            float pressure_effect = std::pow(pressure_ratio, 0.5f);

            return temp_factor * density_effect * pressure_effect;
        }

        float calculate_blade_element_thrust(float radius, float chord, float pitch_angle,
                                           float angular_velocity, float density,
                                           float local_velocity) {
            constexpr float LIFT_SLOPE = 2.0f * PI;

            float tangential_velocity = angular_velocity * radius;
            float resultant_velocity = std::sqrt(tangential_velocity * tangential_velocity +
                                               local_velocity * local_velocity);
            float local_aoa = pitch_angle - std::atan2(local_velocity, tangential_velocity);

            float cl = std::clamp(LIFT_SLOPE * local_aoa, -1.5f, 1.5f);

            return 0.5f * density * resultant_velocity * resultant_velocity * chord * cl;
        }

        float calculate_induced_velocity(float thrust, float air_density, float disc_area) {
            return std::sqrt(thrust / (2.0f * air_density * disc_area));
        }

        float calculate_thrust(const rotor_data* data, const AtmosphericConditions& conditions) {
            if (!data || !data->is_valid) return 0.0f;

            const float omega = data->currentRPM * RPM_TO_RAD;
            if (omega <= 0.0f) return 0.0f;

            // Get forward velocity
            float forward_velocity = 0.0f;
            if (data->rigidBody) {
                btVector3 velocity = data->rigidBody->getLinearVelocity();
                forward_velocity = velocity.length();
            }

            // Basic thrust calculation using blade element theory
            constexpr int ELEMENTS_PER_BLADE = 10;
            const float dr = data->bladeRadius / ELEMENTS_PER_BLADE;
            const float blade_chord = 0.1f * data->bladeRadius;
            float total_thrust = 0.0f;

            for (int i = 0; i < ELEMENTS_PER_BLADE; ++i) {
                float r = (i + 0.5f) * dr;
                float local_pitch = data->bladePitch * (1.0f - r/data->bladeRadius);

                float element_thrust = calculate_blade_element_thrust(
                    r, blade_chord, local_pitch, omega, conditions.density, forward_velocity
                );

                total_thrust += element_thrust * dr;
            }

            total_thrust *= static_cast<float>(data->bladeCount);

            // Calculate efficiency factors based on flight conditions
            float density_factor = calculate_density_altitude_factor(conditions);
            float altitude_efficiency = calculate_altitude_efficiency(data->position.getY(), conditions);

            // Reynolds number effects
            float tip_speed = omega * data->bladeRadius;
            float chord = 0.1f * data->bladeRadius;  // Approximate chord length
            float reynolds = (tip_speed * chord * conditions.density) / conditions.viscosity;
            float reynolds_factor = std::min(1.0f, std::log10(reynolds) / 6.0f);

            // Compressibility effects
            float compressibility_factor = 1.0f;
            if (conditions.mach_factor > 0.3f) {
                float mach_sq = std::min(conditions.mach_factor * conditions.mach_factor, 0.9f);
                compressibility_factor = 1.0f / std::sqrt(1.0f - mach_sq);
            }

            return total_thrust * density_factor * altitude_efficiency *
                   reynolds_factor * compressibility_factor;
        }

        float calculate_power(const rotor_data* data, float thrust,
                            const AtmosphericConditions& conditions) {
            if (!data || !data->is_valid) return 0.0f;

            const float omega = data->currentRPM * RPM_TO_RAD;
            if (omega <= 0.0f) return 0.0f;

            // Induced power
            float induced_velocity = calculate_induced_velocity(thrust, conditions.density, data->discArea);
            float induced_power = thrust * induced_velocity;

            // Profile power
            float tip_velocity = omega * data->bladeRadius;
            float average_cd = 0.012f;
            float profile_power = (1.0f/8.0f) * conditions.density * data->discArea *
                                average_cd * std::pow(tip_velocity, 3);

            // Parasitic power
            float forward_velocity = 0.0f;
            if (data->rigidBody) {
                forward_velocity = data->rigidBody->getLinearVelocity().length();
            }
            float parasitic_power = 0.5f * conditions.density * std::pow(forward_velocity, 3) * 0.002f;

            return induced_power + profile_power + parasitic_power;
        }
    }

    void drone_component::calculate_forces(float deltaTime) {
        auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return;

        float altitude = data->position.getY();
        float velocity = data->rigidBody ? data->rigidBody->getLinearVelocity().length() : 0.0f;

        AtmosphericConditions conditions = calculate_atmospheric_conditions(altitude, velocity);
        float thrust = calculate_thrust(data, conditions);

        data->powerConsumption = calculate_power(data, thrust, conditions);

        if (data->rigidBody) {
            float vertical_velocity = data->rigidBody->getLinearVelocity().y();

            float drag_coefficient = 0.5f * (1.0f + 0.2f * conditions.mach_factor);
            float drag = 0.5f * conditions.density * vertical_velocity *
                        std::abs(vertical_velocity) * data->discArea * drag_coefficient;

            btVector3 net_force = data->rotorNormal * (thrust - drag);
            data->rigidBody->applyCentralForce(net_force);
        }
    }

    void drone_component::initialize() {
        auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return;

        data->currentRPM = 0.0f;
        data->powerConsumption = 0.0f;
        data->discArea = PI * data->bladeRadius * data->bladeRadius;

        if (data->bladeCount == 0) data->bladeCount = 2;
        if (data->bladePitch == 0.0f) data->bladePitch = 0.2f;
    }

    void drone_component::set_rpm(float target_rpm) {
        auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return;
        data->currentRPM = target_rpm;
    }

    float drone_component::get_thrust() const {
        const auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return 0.0f;

        float altitude = data->position.getY();
        float velocity = data->rigidBody ? data->rigidBody->getLinearVelocity().length() : 0.0f;
        AtmosphericConditions conditions = calculate_atmospheric_conditions(altitude, velocity);

        return calculate_thrust(data, conditions);
    }

    float drone_component::get_power_consumption() const {
        const auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return 0.0f;
        return data->powerConsumption;
    }

    drone_component create(init_info info, drone_entity::entity entity) {
        info.discArea = PI * info.bladeRadius * info.bladeRadius;
        return drone_component{ pool.create(info, entity) };
    }

    void remove(drone_component c) {
        pool.remove(c.get_id());
    }
}