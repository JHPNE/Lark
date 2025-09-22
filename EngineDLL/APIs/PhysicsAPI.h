#pragma once
#include "EngineCoreAPI.h"
#include "EngineUtilities.h"

#ifdef __cplusplus
extern "C"
{
#endif
    ENGINE_API bool SetEntityPhysic(lark::id::id_type entity_id, const physics_component &physics);
    ENGINE_API bool GetEntityPhysic(lark::id::id_type entity_id, physics_component *physics);
#ifdef __cplusplus
}
#endif
