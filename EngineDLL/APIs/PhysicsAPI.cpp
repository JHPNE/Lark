// EngineDLL/APIs/PhysicsAPI.cpp
#include "PhysicsAPI.h"
#include "PhysicExtension/World/WorldRegistry.h"
#include "EngineUtilities.h"

#define ENGINEDLL_EXPORTS

using namespace lark;

extern "C"
{

    bool SetEntityPhysic(lark::id::id_type entity_id, const physics_component &physics)
    {
        if (!engine::is_entity_valid(entity_id))
            return false;

        auto entity = engine::entity_from_id(entity_id);
        auto physic_comp = entity.physics();
        if (!physic_comp.is_valid())
            return false;

        return true;
    }

    bool GetEntityPhysic(lark::id::id_type entity_id, physics_component *physics)
    {
        if (!engine::is_entity_valid(entity_id))
            return false;

        auto entity = engine::entity_from_id(entity_id);
        auto physic_comp = entity.physics();
        if (!physic_comp.is_valid())
            return false;

        return true;
    }
}