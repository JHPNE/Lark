#pragma once
#include "ComponentCommon.h"

namespace lark::material
{

// Leaving this empty for now since we will be working with materials only in the frontend
struct init_info
{
};

/**
 * @brief Creates a new material component for an entity
 * @param info Initialization information for the material which is nonexistent
 * @param entity The entity that will own this material component
 * @return A new material component instance
 */
component create(init_info info, game_entity::entity entity);

/**
 * @brief Removes a Material component
 * @param t The material component to remove
 */
void remove(component t);

void shutdown();
}