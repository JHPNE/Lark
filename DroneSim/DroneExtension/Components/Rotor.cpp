#include "Rotor.h"
#include "Physics/RotorPhysics.h"
#include <algorithm>
#include <cmath>

namespace lark::rotor {
    namespace {
        using rotor_data = drone_components::component_data<drone_data::RotorBody>;
        drone_components::component_pool<rotor_id, rotor_data> pool;
    }

    void drone_component::calculate_forces(float deltaTime) {
        auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return;

        float altitude = data->rigidBody->getWorldTransform().getOrigin().getY();
        float velocity = data->rigidBody ? data->rigidBody->getLinearVelocity().length() : 0.0f;
        models::AtmosphericConditions conditions = models::calculate_atmospheric_conditions(altitude, velocity);

        // Update physics states
        physics::update_blade_state(data, velocity, conditions, deltaTime);
        physics::update_vortex_state(data, velocity, conditions, deltaTime);
        physics::update_motor_state(data, conditions, deltaTime);

        // Apply environmental effects
        physics::apply_wall_effects(data, velocity, conditions);
        physics::apply_turbulence(data, conditions, deltaTime);

        // Calculate core physics
        const btVector3 weight(0.0f, -data->mass * 9.81f, 0.0f);
        float thrust = physics::calculate_thrust(data, conditions);
        btVector3 thrust_force = data->rotorNormal * thrust;

        data->powerConsumption = physics::calculate_power(data, thrust, conditions);

        // Apply aerodynamic forces
        btVector3 velocity_vec = data->rigidBody->getLinearVelocity();
        float axial_velocity = velocity_vec.dot(data->rotorNormal);

        constexpr float BASE_CD = 0.5f;
        float effective_cd = BASE_CD * (1.0f + std::abs(axial_velocity) / 10.0f);

        float drag_magnitude = 0.5f * conditions.density * effective_cd * data->discArea *
                             axial_velocity * std::abs(axial_velocity);

        btVector3 drag_force = -data->rotorNormal * drag_magnitude;
        btVector3 net_force = thrust_force + drag_force;

        data->rigidBody->applyCentralForce(net_force);
    }

    void drone_component::initialize() {
        auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return;

        data->currentRPM = 0.0f;
        data->powerConsumption = 0.0f;
        data->discArea = models::PI * data->bladeRadius * data->bladeRadius;

        if (data->bladeCount == 0) data->bladeCount = 2;
        if (data->bladePitch == 0.0f) data->bladePitch = 0.2f;

        physics::initialize_blade_properties(data);
        physics::initialize_motor_parameters(data);
    }

    void drone_component::set_rpm(float target_rpm) {
        auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return;
        data->currentRPM = target_rpm;
    }

    float drone_component::get_thrust() const {
        auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return 0.0f;

        float altitude = glm::vec3(data->transform[3]).y;
        float velocity = data->rigidBody ? data->rigidBody->getLinearVelocity().length() : 0.0f;
        models::AtmosphericConditions conditions =
            models::calculate_atmospheric_conditions(altitude, velocity);

        return physics::calculate_thrust(data, conditions);
    }

    float drone_component::get_power_consumption() const {
        const auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return 0.0f;
        return data->powerConsumption;
    }

    drone_component create(init_info info, drone_entity::entity entity) {
        info.discArea = models::PI * info.bladeRadius * info.bladeRadius;
        return drone_component{ pool.create(info, entity) };
    }

    void remove(drone_component c) {
        pool.remove(c.get_id());
    }

    glm::mat4 get_transform(drone_component c) {
        return pool.get_transform(c.get_id());
    }

    void update_transform(drone_component c, glm::mat4& transform) {
        pool.set_transform(c.get_id(), transform);
    }
}