// EngineDLL/APIs/PhysicsAPI.cpp
#include "PhysicsAPI.h"

#define ENGINEDLL_EXPORTS

using namespace lark;

extern "C"
{
    ENGINE_API bool PhysicsInitializeEnvironment(float gravity_x, float gravity_y, float gravity_z,
                                                 float air_density, bool enable_collisions)
    {
        return true;
    }

    ENGINE_API void PhysicsShutdownEnvironment() {}

    ENGINE_API bool PhysicsSetWindConstant(float vx, float vy, float vz) { return true; }

    ENGINE_API bool PhysicsSetWindDryden(float mean_vx, float mean_vy, float mean_vz,
                                         float altitude, float wingspan, float turbulence)
    {
        return true;
    }

    ENGINE_API bool IsEntityPhysicsEnabled(lark::id::id_type entity_id)
    {
        if (!engine::is_entity_valid(entity_id))
            return false;

        game_entity::entity entity{entity_id};
        return entity.physics().is_valid();
    }

    ENGINE_API bool PhysicsSetEntityTrajectoryHover(lark::id::id_type entity_id, float x, float y,
                                                    float z, float yaw)
    {
        if (!engine::is_entity_valid(entity_id))
            return false;

        game_entity::entity entity{entity_id};
        auto physics = entity.physics();
        if (!physics.is_valid())
            return false;

        return true;
    }

    ENGINE_API bool PhysicsSetEntityTrajectoryCircular(lark::id::id_type entity_id, float cx,
                                                       float cy, float cz, float radius,
                                                       float frequency)
    {
        if (!engine::is_entity_valid(entity_id))
            return false;

        return true;
    }

    ENGINE_API bool PhysicsGetEntityState(lark::id::id_type entity_id, float *position,
                                          float *velocity, float *orientation, float *rotor_speeds)
    {
        if (!engine::is_entity_valid(entity_id))
            return false;

        game_entity::entity entity{entity_id};
        auto physics = entity.physics();
        if (!physics.is_valid())
            return false;

        return true;
    }

    ENGINE_API bool PhysicsSetEntityControlMode(lark::id::id_type entity_id, int mode)
    {
        if (!engine::is_entity_valid(entity_id))
            return false;

        game_entity::entity entity{entity_id};
        auto physics = entity.physics();
        if (!physics.is_valid())
            return false;

        return true;
    }
}