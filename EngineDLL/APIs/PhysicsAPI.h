#pragma once
#include "EngineCoreAPI.h"

#ifdef __cplusplus
extern "C" {
#endif

    ENGINE_API bool SetEntityPhysicsEnabled(lark::id::id_type entity_id, bool enabled);
    ENGINE_API bool IsEntityPhysicsEnabled(lark::id::id_type entity_id);

#ifdef __cplusplus
}
#endif
