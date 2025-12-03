#pragma once
#include "EngineCoreAPI.h"
#include "EngineUtilities.h"

#ifdef __cplusplus
extern "C"
{
#endif

    ENGINE_API bool SetMaterial(lark::id::id_type entity_id,
                                       const transform_component &transform);
    ENGINE_API bool GetMaterial(lark::id::id_type entity_id,
                                       transform_component *out_transform);
#ifdef __cplusplus
}
#endif