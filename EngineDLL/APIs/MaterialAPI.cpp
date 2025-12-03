#pragma once
#include "TransformAPI.h"

#define ENGINEDLL_EXPORTS

using namespace lark;

extern "C"
{
    ENGINE_API bool SetMaterial(lark::id::id_type entity_id)
    {
        if (!engine::is_entity_valid(entity_id))
            return false;

        auto entity = engine::entity_from_id(entity_id);
        auto material_comp = entity.material();
        if (!material_comp.is_valid())
            return false;

        // NO component functionality for material

        return true;
    }

    ENGINE_API bool GetMaterial(lark::id::id_type entity_id)
    {
        if (!engine::is_entity_valid(entity_id))
            return false;

        auto entity = engine::entity_from_id(entity_id);
        auto material_comp= entity.material();
        if (!material_comp.is_valid())
            return false;

        return true;
    }
}