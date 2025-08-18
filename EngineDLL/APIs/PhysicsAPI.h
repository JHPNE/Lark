#pragma once
#include "EngineCoreAPI.h"

#ifdef __cplusplus
extern "C" {
#endif

    ENGINE_API bool PhysicsInitializeEnvironment(float gravity_x, float gravity_y, float gravity_z, float air_density, bool enable_collisions);
    ENGINE_API void PhysicsShutdownEnvironment();
    ENGINE_API bool PhysicsSetWindConstant(float vx, float vy, float vz);
    ENGINE_API bool PhysicsSetWindDryden(float mean_vx, float mean_vy, float mean_vz, float altitude, float wingspan, float turbulence);

    ENGINE_API bool IsEntityPhysicsEnabled(lark::id::id_type entity_id);
    ENGINE_API bool PhysicsSetEntityTrajectoryHover(lark::id::id_type entity_id, float x, float y, float z, float yaw);

    ENGINE_API bool PhysicsSetEntityTrajectoryCircular(lark::id::id_type entity_id, float cx, float cy, float cz, float radius, float frequency);
    ENGINE_API bool Physics_GetEntityState(lark::id::id_type entity_id, float* position, float* velocity, float* orientation, float* rotor_speeds);

    ENGINE_API bool Physics_SetEntityControlMode(lark::id::id_type entity_id, int mode);

#ifdef __cplusplus
}
#endif
