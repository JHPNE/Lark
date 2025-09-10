#pragma once
#include "EngineCoreAPI.h"
#include <Id.h>

#ifdef __cplusplus
extern "C" {
#endif
    ENGINE_API lark::id::id_type CreateGameEntity(game_entity_descriptor* e);
    ENGINE_API bool RemoveGameEntity(lark::id::id_type id);
    ENGINE_API bool UpdateGameEntity(lark::id::id_type id, game_entity_descriptor* e);
#ifdef __cplusplus
}
#endif