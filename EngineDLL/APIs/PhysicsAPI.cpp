// EngineDLL/APIs/PhysicsAPI.cpp
#include "PhysicsAPI.h"
#include "PhysicExtension/World/WorldRegistry.h"
#include "EngineUtilities.h"

#define ENGINEDLL_EXPORTS

using namespace lark;

extern "C"
{
    ENGINE_API bool SetWorldSettings(glm::vec3 gravity, wind windtype)
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

        // Set gravity
        dynamics_world->setGravity(btVector3(gravity.x, gravity.y, gravity.z));

        // Set wind using the chooseWind function
        auto wind_system = engine::chooseWind(windtype);
        world->set_wind(wind_system);

        return true;
    }
}