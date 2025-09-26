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

    bool SetWorldSettings(wind windtype)
    {

        auto* world = physics::WorldRegistry::instance().get_active_world();
        if (!world)
        {
            return false;
        }

        auto* dynamics_world = world->dynamics_world();
        if (!dynamics_world)
        {
            return false;
        }

        // Set wind using the chooseWind function
        auto wind_system = engine::chooseWind(windtype);
        world->set_wind(wind_system);

        return true;
    }

    bool SetWind(wind_type type, glm::vec3 windVec, glm::vec3 windAmp, glm::vec3 windFreq){
        wind wind{};
        wind.type = type;
        wind.amplitudes = windAmp;
        wind.w = windVec;
        wind.frequencies = windFreq;

        auto wind_system = engine::chooseWind(wind);

        auto* world = physics::WorldRegistry::instance().get_active_world();
        if (!world)
        {
            physics::WorldRegistry::instance().set_pending_wind(wind_system);
        }
        else
        {
            world->set_wind(wind_system);
        }

        return true;
    }

}