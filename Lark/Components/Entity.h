/**
 * @file Entity.h
 * @brief Core entity system that implements the Entity Component System (ECS) pattern
 *
 * This file defines the base entity system and component management functionality.
 * It provides mechanisms for creating, removing, and managing entities and their
 * associated components.
 */

#pragma once
#include "ComponentCommon.h"

namespace lark
{

#define INIT_INFO(component)                                                                       \
    namespace component                                                                            \
    {                                                                                              \
    struct init_info;                                                                              \
    }
INIT_INFO(transform);
INIT_INFO(script);
INIT_INFO(geometry);
INIT_INFO(physics);
#undef INIT_INFO

namespace game_entity
{
/**
 * @struct entity_info
 * @brief Initialization information for creating a new entity
 *
 * Contains pointers to initialization information for each possible
 * component that can be attached to an entity.
 */
struct entity_info
{
    transform::init_info *transform{nullptr}; ///< Transform component initialization info
    script::init_info *script{nullptr};       ///< Script component initialization info
    geometry::init_info *geometry{nullptr};   ///< Geometry component initialization info
    physics::init_info *physics{nullptr};     ///< Physics component initialization info
};

/**
 * @brief Creates a new entity with the specified components
 * @param info Initialization information for the entity's components
 * @return A new entity instance
 *
 * Creates a new entity and attaches components based on the provided
 * initialization information. At minimum, a transform component is required.
 */
entity create(entity_info info);

/**
 * @brief Removes an entity and all its components
 * @param id ID of the entity to remove
 *
 * Properly cleans up all components attached to the entity and
 * removes it from the active entities list.
 */
void remove(entity_id id);

/**
 * @brief updates an entity and all its components
 * @param id ID of the entity to update
 * @param info information that needs to be updated
 *
 * Properly updates components attached to the entity
 */
bool updateEntity(entity_id id, entity_info info);

/**
 * @brief Checks if an entity is still valid and active
 * @param id ID of the entity to check
 * @return true if the entity is alive, false otherwise
 */
bool is_alive(entity_id id);

/**
 * @brief Gets a list of all currently active entities
 * @return Reference to vector containing IDs of all active entities
 */
const util::vector<entity_id> &get_active_entities();
} // namespace game_entity
} // namespace lark
