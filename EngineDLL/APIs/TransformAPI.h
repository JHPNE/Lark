#pragma once
#include "EngineCoreAPI.h"
#include "EngineUtilities.h"

#ifdef __cplusplus
extern "C" {
#endif

    ENGINE_API bool SetEntityTransform(lark::id::id_type entity_id, const transform_component& transform);
    ENGINE_API bool GetEntityTransform(lark::id::id_type entity_id, transform_component* out_transform);
    ENGINE_API bool ResetEntityTransform(lark::id::id_type entity_id);
    ENGINE_API glm::mat4 GetEntityTransformMatrix(lark::id::id_type entity_id);

#ifdef __cplusplus
}
#endif
