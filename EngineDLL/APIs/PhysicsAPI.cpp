// EngineDLL/APIs/PhysicsAPI.cpp
#include "PhysicsAPI.h"
#include "Physics/Environment.h"
#include "Physics/TrajectorySystem.h"

#define ENGINEDLL_EXPORTS

using namespace lark;

extern "C" {
    ENGINE_API bool PhysicsInitializeEnvironment(float gravity_x, float gravity_y, float gravity_z,
                                           float air_density, bool enable_collisions) {
        physics::Settings settings;
        settings.gravity = glm::vec3(gravity_x, gravity_y, gravity_z);
        settings.air_density = air_density;
        settings.enable_collisions = enable_collisions;

        physics::Environment::getInstance().initialize(settings);
        return true;
    }

    ENGINE_API void PhysicsShutdownEnvironment() {
        physics::Environment::getInstance().shutdown();
    }

    ENGINE_API bool PhysicsSetWindConstant(float vx, float vy, float vz) {
        auto wind = std::make_shared<physics::wind::ConstantWind>(glm::vec3(vx, vy, vz));
        physics::Environment::getInstance().setWindProfile(wind);
        return true;
    }

    ENGINE_API bool PhysicsSetWindDryden(float mean_vx, float mean_vy, float mean_vz,
                                         float altitude, float wingspan, float turbulence) {
        physics::wind::DrydenGust::Parameters params;
        params.mean_wind = glm::vec3(mean_vx, mean_vy, mean_vz);
        params.altitude = altitude;
        params.wingspan = wingspan;
        params.turbulence_level = turbulence;

        auto wind = std::make_shared<physics::wind::DrydenGust>(params);
        physics::Environment::getInstance().setWindProfile(wind);
        return true;
    }

    ENGINE_API bool IsEntityPhysicsEnabled(lark::id::id_type entity_id) {
        if (!engine::is_entity_valid(entity_id)) return false;

        game_entity::entity entity{entity_id};
        return entity.physics().is_valid();
    }

    ENGINE_API bool PhysicsSetEntityTrajectoryHover(lark::id::id_type entity_id,
                                                     float x, float y, float z, float yaw) {
        if (!engine::is_entity_valid(entity_id)) return false;

        game_entity::entity entity{entity_id};
        auto physics = entity.physics();
        if (!physics.is_valid()) return false;

        auto trajectory = std::make_shared<physics::trajectory::HoverTrajectory>(
            glm::vec3(x, y, z), yaw
        );
        physics.set_trajectory(trajectory);
        return true;
    }

    ENGINE_API bool PhysicsSetEntityTrajectoryCircular(lark::id::id_type entity_id,
                                                        float cx, float cy, float cz,
                                                        float radius, float frequency) {
        if (!engine::is_entity_valid(entity_id)) return false;

        game_entity::entity entity{entity_id};
        auto physics = entity.physics();
        if (!physics.is_valid()) return false;

        physics::trajectory::CircularTrajectory::Parameters params;
        params.center = glm::vec3(cx, cy, cz);
        params.radius = radius;
        params.height = cz;
        params.frequency = frequency;
        params.yaw_follows_velocity = true;

        auto trajectory = std::make_shared<physics::trajectory::CircularTrajectory>(params);
        physics.set_trajectory(trajectory);
        return true;
    }

    ENGINE_API bool PhysicsGetEntityState(lark::id::id_type entity_id,
                                          float* position, float* velocity,
                                          float* orientation, float* rotor_speeds) {
        if (!engine::is_entity_valid(entity_id)) return false;

        game_entity::entity entity{entity_id};
        auto physics = entity.physics();
        if (!physics.is_valid()) return false;

        auto state = physics.get_state();

        if (position) {
            position[0] = state.position.x;
            position[1] = state.position.y;
            position[2] = state.position.z;
        }

        if (velocity) {
            velocity[0] = state.velocity.x;
            velocity[1] = state.velocity.y;
            velocity[2] = state.velocity.z;
        }

        if (orientation) {
            orientation[0] = state.orientation.x;
            orientation[1] = state.orientation.y;
            orientation[2] = state.orientation.z;
            orientation[3] = state.orientation.w;
        }

        if (rotor_speeds && !state.rotor_speeds.empty()) {
            for (size_t i = 0; i < state.rotor_speeds.size(); ++i) {
                rotor_speeds[i] = state.rotor_speeds[i];
            }
        }

        return true;
    }

    ENGINE_API bool PhysicsSetEntityControlMode(lark::id::id_type entity_id, int mode) {
        if (!engine::is_entity_valid(entity_id)) return false;

        game_entity::entity entity{entity_id};
        auto physics = entity.physics();
        if (!physics.is_valid()) return false;

        physics.set_control_mode(static_cast<drones::ControlMode>(mode));
        return true;
    }
}