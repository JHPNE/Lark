#pragma once
#include "Core/GameLoop.h"
#include "EngineCoreAPI.h"
#include "Geometry.h"
#include "Geometry/MeshPrimitives.h"
#include "Script.h"
#include "Transform.h"

using namespace lark;

namespace engine
{

// Function declarations (NOT definitions)
transform::init_info to_engine_transform(const transform_component &transform);
script::init_info to_engine_script(const script_component &script);
geometry::init_info to_engine_geometry(const geometry_component &geometry);
physics::init_info to_engine_physics(const physics_component &physics);

game_entity::entity entity_from_id(id::id_type id);
bool is_entity_valid(id::id_type id);
void remove_entity(id::id_type id);
void cleanup_engine_systems();

extern std::unique_ptr<GameLoop> g_game_loop;
extern util::vector<bool> active_entities;
} // namespace engine
