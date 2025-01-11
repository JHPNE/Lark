#include "Rotor.h"

namespace lark::rotor {
    namespace {
        using rotor_data = drone_components::component_data<drone_data::RotorBody>;
        drone_components::component_pool<rotor_id, rotor_data> pool;

        constexpr float MAX_RPM = 15000.0f;

        float calculate_thrust_coefficient(const rotor_data* data) {
            if (!data) return 0.0f;

            const float solidity_ratio = static_cast<float>(data->bladeCount) *
                                       data->bladePitch / (glm::pi<float>() * data->bladeRadius);
            const float lift_curve_slope = 5.7f;

            return (solidity_ratio * lift_curve_slope * data->bladePitch) / 4.0f -
                   (solidity_ratio * lift_curve_slope) / 8.0f *
                   std::pow(data->bladePitch, 3);
        }

        btVector3 calculate_induced_velocity(const rotor_data* data, float thrust) {
            if (!data) return btVector3(0, 0, 0);

            const float disc_area = glm::pi<float>() * std::pow(data->bladeRadius, 2);
            const float induced_velocity_magnitude =
                std::sqrt(std::abs(thrust) / (2.0f * data->airDensity * disc_area));

            return data->rotorNormal * induced_velocity_magnitude;
        }
    }

    drone_component create(init_info info, drone_entity::entity entity) {
        return drone_component{ pool.create(info, entity)};
    }

    void remove(drone_component c) {
        pool.remove(c.get_id());
    }

    // Methods implementations for drone_component class
    void drone_component::calculate_forces(float deltaTime) {
        auto* data = pool.get_data(get_id());
        if (!data || !data->rigidBody || !data->is_valid) return;

        // Update position from rigid body
        btTransform trans;
        data->rigidBody->getMotionState()->getWorldTransform(trans);
        data->position = trans.getOrigin();

        const float omega = (data->currentRPM * 2.0f * glm::pi<float>()) / 60.0f;
        const float thrust_coeff = calculate_thrust_coefficient(data);
        const float disc_area = data->discArea;

        const float thrust = thrust_coeff *
                           data->airDensity *
                           disc_area *
                           std::pow(omega * data->bladeRadius, 2.0f);

        const btVector3 induced_vel = calculate_induced_velocity(data, thrust);
        const btVector3 force_vector = data->rotorNormal * thrust;

        // Apply force at center of mass for stability
        data->rigidBody->applyCentralForce(force_vector);

        const float torque_magnitude = thrust * induced_vel.length() / std::max(omega, 0.01f);
        const btVector3 torque = -data->rotorNormal * torque_magnitude;
        data->rigidBody->applyTorque(torque);

        // Update power consumption
        data->powerConsumption = thrust * induced_vel.length();
    }

    void drone_component::set_rpm(float target_rpm) {
        auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return;

        data->currentRPM = std::clamp(target_rpm, 0.0f, MAX_RPM);
    }

    float drone_component::get_thrust() const {
        const auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return 0.0f;

        const float omega = (data->currentRPM * 2.0f * glm::pi<float>()) / 60.0f;
        const float thrust_coeff = calculate_thrust_coefficient(data);

        return thrust_coeff *
               data->airDensity *
               data->discArea *
               std::pow(omega * data->bladeRadius, 2);
    }

    float drone_component::get_power_consumption() const {
        const auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return 0.0f;

        return data->powerConsumption;
    }
};