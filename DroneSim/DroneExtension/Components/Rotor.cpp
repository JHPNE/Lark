#include "Rotor.h"
#include <algorithm>
#include <cmath>

namespace lark::rotor {
    namespace {
        using rotor_data = drone_components::component_data<drone_data::RotorBody>;
        drone_components::component_pool<rotor_id, rotor_data> pool;


        models::TurbulenceState get_turbulence(const rotor_data* data, const models::AtmosphericConditions& conditions, float delta_time) {
            if (!data || !data->rigidBody || !data->rigidBody) return models::TurbulenceState{};

            float velocity = data->rigidBody->getLinearVelocity().length();
            float altitude = data->rigidBody->getWorldTransform().getOrigin().getY();

            return models::calculate_turbulence(altitude, velocity, conditions, delta_time);
        }

        void apply_turbulence(rotor_data* data, const models::TurbulenceState& turbulence) {
            if (!data || !data->is_valid || !data->rigidBody) return;

            btVector3 turbulent_force = btVector3(
                turbulence.velocity.x(),
                turbulence.velocity.y(),
                turbulence.velocity.z()
            ) * data->mass * turbulence.intensity;

            btVector3 turbulent_torque = btVector3(
                turbulence.angular_velocity.x(),
                turbulence.angular_velocity.y(),
                turbulence.angular_velocity.z()
            ) * data->mass * data->bladeRadius * turbulence.intensity;

            //float rpm_factor = data->currentRPM / 1000.0f

            data->rigidBody->applyCentralForce(turbulent_force);
            data->rigidBody->applyTorque(turbulent_torque);
        }

        models::PropWashField get_prop_wash(const rotor_data* data, const models::AtmosphericConditions& conditions, float thrust) {
            models::PropWashField prop_wash {};
            if (!data || !data->is_valid || !data->rigidBody) return prop_wash;
            prop_wash = calculate_prop_wash(data->rotorNormal, data->currentRPM, data->discArea, data->bladeRadius, data->bladeCount, conditions, thrust);
            return prop_wash;
        }




        float calculate_thrust(const rotor_data* data, const models::AtmosphericConditions& conditions) {
            if (!data || !data->is_valid) return 0.0f;

            const float omega = data->currentRPM * models::RPM_TO_RAD;
            if (omega <= 0.0f) return 0.0f;

            // Get forward velocity & height
            float forward_velocity = 0.0f;
            float altitude = 0.0f;
            if (data->rigidBody) {
                btVector3 velocity = data->rigidBody->getLinearVelocity();
                forward_velocity = velocity.length();
                altitude = data->rigidBody->getWorldTransform().getOrigin().getY();
            }

            // Blade element method for thrust calculation
            constexpr int ELEMENTS_PER_BLADE = 10;
            const float dr = data->bladeRadius / ELEMENTS_PER_BLADE;
            const float blade_chord = 0.1f * data->bladeRadius;
            float total_thrust = 0.0f;

            for (int i = 0; i < ELEMENTS_PER_BLADE; ++i) {
                float r = (i + 0.5f) * dr;
                float local_pitch = data->bladePitch * (1.0f - r / data->bladeRadius);

                // Element thrust
                float tangential_velocity = omega * r;
                float resultant_velocity = std::sqrt(tangential_velocity * tangential_velocity + forward_velocity * forward_velocity);
                float aoa = local_pitch - std::atan2(forward_velocity, tangential_velocity);
                constexpr float LIFT_SLOPE = 2.0f * models::PI;
                float cl = std::clamp(LIFT_SLOPE * aoa, -1.5f, 1.5f);

                total_thrust += 0.5f * conditions.density * resultant_velocity * resultant_velocity * blade_chord * cl * dr;
            }

            total_thrust *= data->bladeCount;

            // Ground Effect
            float rotor_diameter = 2.0f * data->bladeRadius;
            float normalized_height = altitude / rotor_diameter;

            if (normalized_height < 2.0f) {
                // Cheeseman Benett model for ground effect
                // IGE_THRUST = OGE_Thrust * (1 / (1 - (R/4h)²))
                float ground_effect_factor = 1.0f;
                if (normalized_height > 0.1f) {
                    ground_effect_factor = 1.0f/ (1.0f - std::pow(1.0f / (4.0f * normalized_height), 2));
                    ground_effect_factor = std::min(ground_effect_factor, 1.4f);
                }

                // Smooth transition for very low heights
                if (normalized_height < 0.1f) {
                    ground_effect_factor = 1.4f * (normalized_height / 0.1f);
                }

                total_thrust *= ground_effect_factor;

                // Circulation effects
                if (normalized_height < 0.3f) {
                    float recirculation_factor = 1.0f - (normalized_height / 0.3f) * 0.1f;
                    total_thrust *= recirculation_factor;
                }
            }

            return total_thrust;
        }

        void apply_wash(const rotor_data* data, const models::AtmosphericConditions& conditions) {
             if (data->rigidBody) {
                btVector3 total_wash_velocity(0,0,0);
                btVector3 total_wash_vorticity(0,0,0);

                const auto& all_rotors = pool.get_all_components();
                for (const auto& other_rotor : all_rotors) {
                    if (other_rotor.drone_id == data->drone_id) continue;

                    const auto* other_data = pool.get_data(other_rotor.drone_id);
                    if (!other_data || !other_data->is_valid) continue;

                    models::PropWashField other_wash = get_prop_wash(other_data, conditions,
                                                                           calculate_thrust(other_data, conditions));

                    float influence = calculate_prop_wash_influence(other_wash,
                        other_data->rigidBody->getWorldTransform().getOrigin(),
                        data->rigidBody->getWorldTransform().getOrigin(),
                        other_data->bladeRadius);

                    total_wash_velocity += other_wash.velocity * influence;
                    total_wash_vorticity += other_wash.vorticity * influence;
                }

                // Apply accumulated prop wash effects
                btVector3 wash_force = total_wash_velocity * data->mass * 0.5f;
                btVector3 wash_torque = total_wash_vorticity * data->mass * data->bladeRadius * 0.3f;

                data->rigidBody->applyCentralForce(wash_force);
                data->rigidBody->applyTorque(wash_torque);
            }
        }

        float calculate_power(const rotor_data* data, float thrust, const models::AtmosphericConditions& conditions) {
            if (!data || !data->is_valid) return 0.0f;

            const float omega = data->currentRPM * models::RPM_TO_RAD;
            if (omega <= 0.0f) return 0.0f;

            // Induced velocity
            float induced_velocity = std::sqrt(thrust / (2.0f * conditions.density * data->discArea));

            // Power components
            float induced_power = thrust * induced_velocity;
            float profile_power = (1.0f / 8.0f) * conditions.density * data->discArea * 0.012f * std::pow(omega * data->bladeRadius, 3);
            float forward_velocity = data->rigidBody ? data->rigidBody->getLinearVelocity().length() : 0.0f;
            float parasitic_power = 0.5f * conditions.density * std::pow(forward_velocity, 3) * 0.002f;

            return induced_power + profile_power + parasitic_power;
        }
    }

    void drone_component::calculate_forces(float deltaTime) {
        auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return;

        float altitude = data->rigidBody->getWorldTransform().getOrigin().getY();
        float velocity = data->rigidBody ? data->rigidBody->getLinearVelocity().length() : 0.0f;
        models::AtmosphericConditions conditions = models::calculate_atmospheric_conditions(altitude, velocity);

        // Calculate blade flapping dynamics
        data->blade_state = models::calculate_blade_state(
            data->blade_properties,
            data->currentRPM * models::RPM_TO_RAD,
            velocity,
            conditions.density,
            data->bladePitch,  // collective pitch
            0.0f,              // cyclic pitch (can be controlled separately)
            0.0f,              // shaft tilt
            deltaTime
        );

        // Calculate tip vortex effects
        models::VortexParameters vortex_params;
        vortex_params.blade_tip_speed = data->currentRPM * models::RPM_TO_RAD * data->bladeRadius;
        vortex_params.blade_chord = 0.1f * data->bladeRadius;
        vortex_params.effective_aoa = data->bladePitch;
        vortex_params.blade_span = data->bladeRadius;
        vortex_params.blade_count = data->bladeCount;

        data->vortex_state = models::calculate_tip_vortex(
            vortex_params,
            conditions.density,
            data->currentRPM * models::RPM_TO_RAD,
            velocity,
            data->rigidBody->getWorldTransform().getOrigin(),
            data->rigidBody->getWorldTransform().getOrigin() + btVector3(0, -data->bladeRadius, 0),
            deltaTime
        );

        // Calculate motor dynamics
        data->motor_state = models::calculate_motor_state(
            data->motor_parameters,
            data->currentRPM,
            data->powerConsumption / (data->currentRPM * models::RPM_TO_RAD), // estimate load torque
            conditions.temperature,
            deltaTime
        );

        // Calculate wall effects if near obstacles
        const float wall_detection_radius = 2.0f * data->bladeRadius;
        btVector3 rotor_pos = data->rigidBody->getWorldTransform().getOrigin();
        btVector3 rotor_vel = data->rigidBody->getLinearVelocity();

        // Simple wall detection using raycasts
        btCollisionWorld::ClosestRayResultCallback ray_callback(
            rotor_pos,
            rotor_pos + btVector3(wall_detection_radius, 0, 0)
        );

        if (data->dynamics_world) {
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
                    rotor_vel,
                    data->bladePitch
                );

                // Apply wall effect forces
                data->rigidBody->applyCentralForce(data->wall_state.induced_force);
                data->rigidBody->applyTorque(data->wall_state.induced_moment);
            }
        }

        // Applies Turbulence
        models::TurbulenceState turbulence = get_turbulence(data, conditions, deltaTime);
        apply_turbulence(data, turbulence);

        float thrust = calculate_thrust(data, conditions);

        // Applies Wash
        apply_wash(data, conditions);

        data->powerConsumption = calculate_power(data, thrust, conditions);

        if (data->rigidBody) {
            // Get velocity vector and calculate relative airflow
            const btVector3& velocity = data->rigidBody->getLinearVelocity();

            // Project velocity onto rotor normal to get axial component
            float axial_velocity = velocity.dot(data->rotorNormal);

            // Calculate drag coefficient based on angle of attack
            // Using a simplified model for now - could be enhanced with lookup tables
            constexpr float BASE_CD = 0.5f;  // Base drag coefficient for a flat disc
            float effective_cd = BASE_CD * (1.0f + std::abs(axial_velocity) / 10.0f); // Increases with speed

            // Calculate drag force using proper aerodynamic formula
            float drag_magnitude = 0.5f * conditions.density * effective_cd * data->discArea *
                                 axial_velocity * std::abs(axial_velocity);

            // Apply drag force in direction opposite to motion
            btVector3 drag_force = -data->rotorNormal * drag_magnitude;

            // Combine thrust and drag
            btVector3 net_force = (data->rotorNormal * thrust) + drag_force;

            // Apply the combined force
            data->rigidBody->applyCentralForce(net_force);
        }
    }

    void drone_component::initialize() {
        auto* data = pool.get_data(get_id());
        if (!data || !data->is_valid) return;

        data->currentRPM = 0.0f;
        data->powerConsumption = 0.0f;
        data->discArea = models::PI * data->bladeRadius * data->bladeRadius;

        // Initialize blade properties
        data->blade_properties.mass = data->mass / data->bladeCount;
        data->blade_properties.hinge_offset = 0.05f * data->bladeRadius;
        data->blade_properties.lock_number = 5.0f;  // Typical value for small rotors
        data->blade_properties.spring_constant = 1000.0f;
        data->blade_properties.natural_frequency = std::sqrt(data->blade_properties.spring_constant /
                                                           data->blade_properties.mass);
        data->blade_properties.blade_grip = 0.95f * data->bladeRadius;

        // Initialize motor parameters
        data->motor_parameters.kv_rating = 1000.0f;  // Example KV rating
        data->motor_parameters.resistance = 0.1f;     // Example resistance in ohms
        data->motor_parameters.inductance = 0.0001f;  // Example inductance in H
        data->motor_parameters.inertia = 0.0001f;    // Example rotor inertia
        data->motor_parameters.thermal_resistance = 10.0f;  // Example thermal resistance
        data->motor_parameters.thermal_capacity = 100.0f;   // Example thermal capacity
        data->motor_parameters.voltage = 11.1f;       // Example 3S LiPo voltage
        data->motor_parameters.max_current = 30.0f;   // Example max current

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

        float altitude = glm::vec3(data->transform[3]).y;
        float velocity = data->rigidBody ? data->rigidBody->getLinearVelocity().length() : 0.0f;
        models::AtmosphericConditions conditions = models::calculate_atmospheric_conditions(altitude, velocity);

        return calculate_thrust(data, conditions);
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