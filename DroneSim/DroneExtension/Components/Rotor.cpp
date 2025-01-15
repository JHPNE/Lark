#include "Rotor.h"
#include <algorithm>
#include <cmath>

namespace lark::rotor {
    namespace {
        constexpr float PI = glm::pi<float>();
        constexpr float RAD_TO_RPM = 60.0f / (2.0f * PI);
        constexpr float RPM_TO_RAD = (2.0f * PI) / 60.0f;
        constexpr float GRAVITY = 9.81f;

        using rotor_data = drone_components::component_data<drone_data::RotorBody>;
        drone_components::component_pool<rotor_id, rotor_data> pool;

        float calculate_air_density(float altitude) {
            constexpr float BASE_TEMPERATURE = 288.15f; // K (15°C at sea level)
            constexpr float TEMPERATURE_LAPSE_RATE = -0.0065f; // K/m
            constexpr float GAS_CONSTANT = 287.05f; // J/(kg·K)
            constexpr float PRESSURE_SEA_LEVEL = 101325.0f; // Pa

            float temperature = BASE_TEMPERATURE + TEMPERATURE_LAPSE_RATE * altitude;
            float exponent = -GRAVITY / (GAS_CONSTANT * TEMPERATURE_LAPSE_RATE);
            float pressure = PRESSURE_SEA_LEVEL * std::pow(temperature / BASE_TEMPERATURE, exponent);

            return pressure / (GAS_CONSTANT * temperature);
        }

        float calculate_advance_ratio(float forward_velocity, float rpm, float diameter) {
            const float angular_velocity = rpm * RPM_TO_RAD;
            return forward_velocity / (angular_velocity * diameter);
        }

        float calculate_induced_velocity(float thrust, float air_density, float disc_area) {
            return std::sqrt(thrust / (2.0f * air_density * disc_area));
        }

        float calculate_blade_element_thrust(float radius, float chord, float pitch_angle,
                                           float angular_velocity, float air_density,
                                           float local_velocity) {
            constexpr float LIFT_SLOPE = 2.0f * PI; // Theoretical lift slope

            // Calculate local blade element velocity and angle of attack
            float tangential_velocity = angular_velocity * radius;
            float resultant_velocity = std::sqrt(tangential_velocity * tangential_velocity +
                                               local_velocity * local_velocity);
            float local_aoa = pitch_angle - std::atan2(local_velocity, tangential_velocity);

            // Calculate local lift coefficient (simplified)
            float cl = LIFT_SLOPE * local_aoa;

            // Limit cl to reasonable values
            cl = std::clamp(cl, -1.5f, 1.5f);

            // Calculate local thrust
            return 0.5f * air_density * resultant_velocity * resultant_velocity *
                   chord * cl;
        }

        float calculate_blade_stall(float altitude, float pitch_angle, float tip_speed) {
            // Atmospheric properties affecting stall
            constexpr float BASE_TEMPERATURE = 288.15f; // K (15°C at sea level)
            constexpr float TEMPERATURE_LAPSE_RATE = -0.0065f; // K/m

            // Calculate temperature at altitude
            float temperature = BASE_TEMPERATURE + TEMPERATURE_LAPSE_RATE * altitude;

            // Calculate speed of sound at this altitude
            float speed_of_sound = std::sqrt(1.4f * 287.05f * temperature);

            // Calculate local Mach number
            float mach_number = tip_speed / speed_of_sound;

            // Critical angle of attack (decreases with Mach number)
            float critical_aoa = 0.3f - 0.1f * mach_number;

            // Return stall factor (1.0 = no stall, 0.0 = complete stall)
            float stall_factor = 1.0f - std::clamp((pitch_angle - critical_aoa) / critical_aoa, 0.0f, 1.0f);
            return stall_factor;
    }

    float calculate_thrust(const rotor_data* data, float air_density) {
            if (!data || !data->is_valid) return 0.0f;

            const float omega = data->currentRPM * RPM_TO_RAD; // Angular velocity in rad/s
            if (omega <= 0.0f) return 0.0f;

            // Air properties at current altitude
            float altitude = data->position.getY();

            // Calculate temperature and air properties
            constexpr float BASE_TEMPERATURE = 288.15f; // K (15°C at sea level)
            constexpr float TEMPERATURE_LAPSE_RATE = -0.0065f; // K/m
            float temperature = BASE_TEMPERATURE + TEMPERATURE_LAPSE_RATE * altitude;

            // Speed of sound affects blade effectiveness
            float speed_of_sound = std::sqrt(1.4f * 287.05f * temperature);

            // Get forward velocity from rigid body if available
            float forward_velocity = 0.0f;
            if (data->rigidBody) {
                btVector3 velocity = data->rigidBody->getLinearVelocity();
                forward_velocity = velocity.length();
            }

            // Calculate advance ratio
            float diameter = 2.0f * data->bladeRadius;
            float advance_ratio = calculate_advance_ratio(forward_velocity, data->currentRPM, diameter);

            // Blade element discretization
            constexpr int ELEMENTS_PER_BLADE = 10; // Number of calculation segments per blade
            const float dr = data->bladeRadius / ELEMENTS_PER_BLADE;
            const float blade_chord = 0.1f * data->bladeRadius; // Simplified chord length
            float total_thrust = 0.0f;

            // Calculate thrust for each blade element
            // Each physical blade is divided into calculation segments
            for (int i = 0; i < ELEMENTS_PER_BLADE; ++i) {
                float r = (i + 0.5f) * dr; // Radius at element center
                float local_pitch = data->bladePitch * (1.0f - r/data->bladeRadius); // Linear twist

                float element_thrust = calculate_blade_element_thrust(
                    r, blade_chord, local_pitch, omega, air_density, forward_velocity
                );

                total_thrust += element_thrust * dr;
            }

            // Multiply by number of blades
            total_thrust *= static_cast<float>(data->bladeCount);

            // Apply efficiency factors
            float tip_loss_factor = 0.97f - 0.0215f * advance_ratio; // Prandtl tip loss factor

            // Calculate Mach number for blade tips
            float tip_velocity = omega * data->bladeRadius;
            float mach_number = tip_velocity / speed_of_sound;

            // Calculate Reynolds number effects
            float kinematic_viscosity = 1.46e-5f * std::sqrt(temperature / BASE_TEMPERATURE); // Viscosity varies with temperature
            float reynolds = (tip_velocity * blade_chord) / kinematic_viscosity;
            float reynolds_factor = std::min(1.0f, std::log10(reynolds) / 6.0f);

            // Compressibility effects become significant at high Mach numbers
            float compressibility_factor = mach_number > 0.3f ?
                1.0f / std::sqrt(1.0f - std::min(0.9f, mach_number * mach_number)) : 1.0f;

            // Density effects on thrust
            float density_ratio = air_density / 1.225f;
            float density_factor = std::pow(density_ratio, 1.5f); // Non-linear density impact

            // Blade effectiveness factor based on air density and temperature
            float blade_effectiveness = std::exp(-std::pow(1.0f - density_ratio, 2.0f)) *
                                     std::exp(-std::abs(temperature - BASE_TEMPERATURE) / 100.0f);

            // Local speed of sound affects maximum achievable thrust
            float sonic_limitation = std::exp(-std::max(0.0f, mach_number - 0.7f));

            total_thrust *= tip_loss_factor * density_factor * reynolds_factor *
                           blade_effectiveness * sonic_limitation * compressibility_factor;

            return total_thrust;
        }

        float calculate_power(const rotor_data* data, const float thrust, float air_density) {
            if (!data || !data->is_valid) return 0.0f;

            const float omega = data->currentRPM * RPM_TO_RAD;
            if (omega <= 0.0f) return 0.0f;

            // Induced power (from momentum theory)
            float induced_velocity = calculate_induced_velocity(thrust, air_density, data->discArea);
            float induced_power = thrust * induced_velocity;

            // Profile power (from blade element theory)
            float tip_velocity = omega * data->bladeRadius;
            float average_cd = 0.012f; // Average drag coefficient
            float profile_power = (1.0f/8.0f) * air_density * data->discArea *
                                average_cd * std::pow(tip_velocity, 3);

            // Parasitic power (from forward flight)
            float forward_velocity = 0.0f;
            if (data->rigidBody) {
                btVector3 velocity = data->rigidBody->getLinearVelocity();
                forward_velocity = velocity.length();
            }
            float parasitic_power = 0.5f * air_density * forward_velocity * forward_velocity *
                                   forward_velocity * 0.002f; // Simplified parasitic drag

            float total_power = induced_power + profile_power + parasitic_power;

            // Altitude efficiency loss
            float altitude_factor = std::exp(-data->position.getY() / 15000.0f);
            total_power /= altitude_factor;

            return total_power;
        }
    }

    void drone_component::calculate_forces(float deltaTime) {
        auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return;

        // Get current altitude from position
        const float altitude = data->position.getY();
        const float air_density = calculate_air_density(altitude);

        // Calculate thrust and power
        const float thrust = calculate_thrust(data, air_density);
        data->powerConsumption = calculate_power(data, thrust, air_density);

        // Apply forces to rigid body if available
        if (data->rigidBody) {
            // Get vertical velocity for drag calculation
            float vertical_velocity = data->rigidBody->getLinearVelocity().y();

            // Calculate drag (simplified model)
            float drag_coefficient = 0.5f;
            float drag = 0.5f * air_density * vertical_velocity * std::abs(vertical_velocity) *
                        data->discArea * drag_coefficient;

            // Apply net force considering thrust direction
            btVector3 net_force = data->rotorNormal * (thrust - drag);
            data->rigidBody->applyCentralForce(net_force);
        }
    }

    void drone_component::initialize() {
        auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return;

        // Initialize rotor parameters
        data->currentRPM = 0.0f;
        data->powerConsumption = 0.0f;
        data->discArea = PI * data->bladeRadius * data->bladeRadius;

        // Set default blade properties if not already set
        if (data->bladeCount == 0) data->bladeCount = 2;
        if (data->bladePitch == 0.0f) data->bladePitch = 0.2f; // Approximately 11.5 degrees
    }

    void drone_component::set_rpm(float target_rpm) {
        auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return;

        // Apply realistic motor response limitations
        constexpr float MAX_RPM_CHANGE_RATE = 1000.0f; // RPM per second
        const float current_rpm = data->currentRPM;
        const float rpm_diff = target_rpm - current_rpm;
        const float rpm_change = std::clamp(rpm_diff, -MAX_RPM_CHANGE_RATE, MAX_RPM_CHANGE_RATE);

        //data->currentRPM = std::max(0.0f, current_rpm + rpm_change);
        data->currentRPM = target_rpm;
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
        // Initialize disc area before creation
        info.discArea = PI * info.bladeRadius * info.bladeRadius;
        return drone_component{ pool.create(info, entity) };
    }

    void remove(drone_component c) {
        pool.remove(c.get_id());
    }
}