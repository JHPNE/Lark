#include "RotorPhysics.h"

namespace lark::rotor::physics {

    float calculate_thrust(drone_data::RotorBody* data,
                          const models::AtmosphericConditions& conditions) {
        if (!data || !data->rigidBody) return 0.0f;

        const float omega = data->currentRPM * models::RPM_TO_RAD;
        if (omega <= 0.0f) return 0.0f;

        // Blade element method calculations
        constexpr int ELEMENTS_PER_BLADE = 10;
        const float dr = data->bladeRadius / ELEMENTS_PER_BLADE;
        const float blade_chord = 0.1f * data->bladeRadius;
        float total_thrust = 0.0f;

        // Calculate basic thrust
        for (int i = 0; i < ELEMENTS_PER_BLADE; ++i) {
            float r = (i + 0.5f) * dr;
            float local_pitch = data->bladePitch * (1.0f - r / data->bladeRadius);

            float tangential_velocity = omega * r;
            float forward_velocity = data->rigidBody->getLinearVelocity().length();
            float resultant_velocity = std::sqrt(tangential_velocity * tangential_velocity +
                                               forward_velocity * forward_velocity);

            float aoa = local_pitch - std::atan2(forward_velocity, tangential_velocity);
            constexpr float LIFT_SLOPE = 2.0f * models::PI;
            float cl = std::clamp(LIFT_SLOPE * aoa, -1.5f, 1.5f);

            total_thrust += 0.5f * conditions.density * resultant_velocity * resultant_velocity *
                           blade_chord * cl * dr;
        }

        float base_thrust = total_thrust * data->bladeCount;

        // Setup ground effect parameters
        models::GroundEffectParams ge_params{};
        ge_params.rotor_radius = data->bladeRadius;
        ge_params.disk_loading = base_thrust / data->discArea;
        ge_params.thrust_coefficient = base_thrust /
            (0.5f * conditions.density * std::pow(omega * data->bladeRadius, 2) * data->discArea);
        ge_params.collective_pitch = data->bladePitch;
        ge_params.position = data->rigidBody->getWorldTransform().getOrigin();
        ge_params.velocity = data->rigidBody->getLinearVelocity();

        // Calculate ground effect
        float altitude = data->rigidBody->getWorldTransform().getOrigin().getY();
        models::GroundEffectState ge_state = models::calculate_ground_effect(ge_params, altitude, conditions);

        // Store ground effect state for other calculations
        data->ground_effect_state = ge_state;

        return base_thrust * ge_state.thrust_multiplier;
    }

    float calculate_power(const drone_data::RotorBody* data, float thrust,
                         const models::AtmosphericConditions& conditions) {
        if (!data || !data->rigidBody) return 0.0f;

        const float omega = data->currentRPM * models::RPM_TO_RAD;
        if (omega <= 0.0f) return 0.0f;

        // Calculate induced velocity
        float induced_velocity = std::sqrt(thrust / (2.0f * conditions.density * data->discArea));

        // Calculate power components
        float induced_power = thrust * induced_velocity;
        float profile_power = (1.0f / 8.0f) * conditions.density * data->discArea * 0.012f *
                             std::pow(omega * data->bladeRadius, 3);

        float forward_velocity = data->rigidBody->getLinearVelocity().length();
        float parasitic_power = 0.5f * conditions.density * std::pow(forward_velocity, 3) * 0.002f;

        return induced_power + profile_power + parasitic_power;
    }

    void update_blade_state(drone_data::RotorBody* data, float velocity,
                           const models::AtmosphericConditions& conditions, float deltaTime) {
        if (!data || !data->rigidBody) return;

        data->blade_state = models::calculate_blade_state(
            data->blade_properties,
            data->currentRPM * models::RPM_TO_RAD,
            velocity,
            conditions.density,
            data->bladePitch,
            0.0f,  // cyclic pitch
            0.0f,  // shaft tilt
            deltaTime
        );
    }

    void update_vortex_state(drone_data::RotorBody* data, float velocity,
                            const models::AtmosphericConditions& conditions, float deltaTime) {
        if (!data || !data->rigidBody) return;

        models::VortexParameters params{};
        params.blade_tip_speed = data->currentRPM * models::RPM_TO_RAD * data->bladeRadius;
        params.blade_chord = 0.1f * data->bladeRadius;
        params.effective_aoa = data->bladePitch;
        params.blade_span = data->bladeRadius;
        params.blade_count = data->bladeCount;

        btVector3 rotor_pos = data->rigidBody->getWorldTransform().getOrigin();
        data->vortex_state = models::calculate_tip_vortex(
            params,
            conditions.density,
            data->currentRPM * models::RPM_TO_RAD,
            velocity,
            rotor_pos,
            rotor_pos + btVector3(0, -data->bladeRadius, 0),
            deltaTime
        );
    }


    void update_motor_state(drone_data::RotorBody* data,
                           const models::AtmosphericConditions& conditions, float deltaTime) {
        if (!data || !data->rigidBody) return;

        float load_torque = data->powerConsumption / (data->currentRPM * models::RPM_TO_RAD);

        data->motor_state = models::calculate_motor_state(
            data->motor_parameters,
            data->currentRPM,
            load_torque,
            conditions.temperature,
            deltaTime
        );
    }

    void apply_wall_effects(drone_data::RotorBody* data, float velocity,
                           const models::AtmosphericConditions& conditions) {
        if (!data || !data->rigidBody || !data->dynamics_world) return;

        const float wall_detection_radius = 2.0f * data->bladeRadius;
        btVector3 rotor_pos = data->rigidBody->getWorldTransform().getOrigin();

        btCollisionWorld::ClosestRayResultCallback ray_callback(
            rotor_pos,
            rotor_pos + btVector3(wall_detection_radius, 0, 0)
        );

        data->dynamics_world->rayTest(rotor_pos,
            rotor_pos + btVector3(wall_detection_radius, 0, 0),
            ray_callback);

        if (ray_callback.hasHit()) {
            models::WallParameters wall_params;
            wall_params.wall_normal = ray_callback.m_hitNormalWorld;
            wall_params.wall_distance = ray_callback.m_closestHitFraction * wall_detection_radius;
            wall_params.rotor_radius = data->bladeRadius;
            wall_params.disk_loading = data->blade_state.disk_loading;
            wall_params.thrust = calculate_thrust(data, conditions);

            data->wall_state = models::calculate_wall_effect(
                wall_params,
                conditions.density,
                velocity,
                rotor_pos,
                data->rigidBody->getLinearVelocity(),
                data->bladePitch
            );

            data->rigidBody->applyCentralForce(data->wall_state.induced_force);
            data->rigidBody->applyTorque(data->wall_state.induced_moment);
        }
    }

    void apply_turbulence(drone_data::RotorBody* data,
                         const models::AtmosphericConditions& conditions, float deltaTime) {
        if (!data || !data->rigidBody) return;

        float velocity = data->rigidBody->getLinearVelocity().length();
        float altitude = data->rigidBody->getWorldTransform().getOrigin().getY();

        models::TurbulenceState turbulence = models::calculate_turbulence(
            altitude,
            velocity,
            conditions,
            deltaTime
        );

        btVector3 turbulent_force = btVector3(
            turbulence.velocity.x(),
            turbulence.velocity.y(),
            turbulence.velocity.z()
        ) * data->mass;

        btVector3 turbulent_torque = btVector3(
            turbulence.angular_velocity.x(),
            turbulence.angular_velocity.y(),
            turbulence.angular_velocity.z()
        ) * data->mass * data->bladeRadius;

        // Apply forces with additional damping at high altitudes
        data->rigidBody->applyCentralForce(turbulent_force);
        data->rigidBody->applyTorque(turbulent_torque);
    }

    void apply_prop_wash(drone_data::RotorBody* data,
                        const models::AtmosphericConditions& conditions,
                        const std::vector<drone_data::RotorBody*>& other_rotors) {
        if (!data || !data->rigidBody) return;

        btVector3 total_wash_velocity(0, 0, 0);
        btVector3 total_wash_vorticity(0, 0, 0);
        btVector3 rotor_pos = data->rigidBody->getWorldTransform().getOrigin();

        for (auto* other_rotor : other_rotors) {
            if (other_rotor == data) continue;

            float thrust = calculate_thrust(other_rotor, conditions);
            models::PropWashField wash = models::calculate_prop_wash(
                other_rotor->rotorNormal,
                other_rotor->currentRPM,
                other_rotor->discArea,
                other_rotor->bladeRadius,
                other_rotor->bladeCount,
                conditions,
                thrust
            );

            float influence = models::calculate_prop_wash_influence(
                wash,
                other_rotor->rigidBody->getWorldTransform().getOrigin(),
                rotor_pos,
                other_rotor->bladeRadius
            );

            total_wash_velocity += wash.velocity * influence;
            total_wash_vorticity += wash.vorticity * influence;
        }

        // Apply accumulated wash effects
        btVector3 wash_force = total_wash_velocity * data->mass * 0.5f;
        btVector3 wash_torque = total_wash_vorticity * data->mass * data->bladeRadius * 0.3f;

        data->rigidBody->applyCentralForce(wash_force);
        data->rigidBody->applyTorque(wash_torque);
    }

    void initialize_blade_properties(drone_data::RotorBody* data) {
        if (!data) return;

        data->blade_properties.mass = data->mass / data->bladeCount;
        data->blade_properties.hinge_offset = 0.05f * data->bladeRadius;
        data->blade_properties.lock_number = 5.0f;
        data->blade_properties.spring_constant = 1000.0f;
        data->blade_properties.natural_frequency =
            std::sqrt(data->blade_properties.spring_constant / data->blade_properties.mass);
        data->blade_properties.blade_grip = 0.95f * data->bladeRadius;
    }

    void initialize_motor_parameters(drone_data::RotorBody* data) {
        if (!data) return;

        data->motor_parameters.kv_rating = 1000.0f;
        data->motor_parameters.resistance = 0.1f;
        data->motor_parameters.inductance = 0.0001f;
        data->motor_parameters.inertia = 0.0001f;
        data->motor_parameters.thermal_resistance = 10.0f;
        data->motor_parameters.thermal_capacity = 100.0f;
        data->motor_parameters.voltage = 11.1f;
        data->motor_parameters.max_current = 30.0f;
    }
} // namespace lark::rotor::physics