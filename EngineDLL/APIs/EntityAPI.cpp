#include "EntityAPI.h"

#define ENGINEDLL_EXPORTS

using namespace lark;

extern "C"
{
    ENGINE_API id::id_type CreateGameEntity(game_entity_descriptor *e)
    {
        assert(e);
        transform::init_info transform_info = engine::to_engine_transform(e->transform);
        script::init_info script_info = engine::to_engine_script(e->script);
        geometry::init_info geometry_info = engine::to_engine_geometry(e->geometry);
        physics::init_info physics_info = engine::to_engine_physics(e->physics);
        drone::init_info drone_info = engine::to_engine_drone(e->drone);

        game_entity::entity_info entity_info{&transform_info, &script_info, &geometry_info, &physics_info, &drone_info};

        auto entity = game_entity::create(entity_info);
        if (entity.is_valid())
        {
            auto index = id::index(entity.get_id());
            if (index >= engine::active_entities.size())
            {
                engine::active_entities.resize(index + 1, false);
            }
            engine::active_entities[index] = true;
        }
        return entity.get_id();
    }

    ENGINE_API bool RemoveGameEntity(id::id_type id)
    {
        engine::remove_entity(id);
        return true;
    }

    ENGINE_API bool UpdateGameEntity(id::id_type id, game_entity_descriptor *e)
    {
        assert(e);
        transform::init_info transform_info = engine::to_engine_transform(e->transform);
        script::init_info script_info = engine::to_engine_script(e->script);
        geometry::init_info geometry_info = engine::to_engine_geometry(e->geometry);
        physics::init_info physics_info = engine::to_engine_physics(e->physics);
        drone::init_info drone_info = engine::to_engine_drone(e->drone);

        game_entity::entity_info entity_info{&transform_info, &script_info, &geometry_info, &physics_info, &drone_info};

        return game_entity::updateEntity(id, entity_info);
    }
}
