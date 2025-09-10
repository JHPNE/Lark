#pragma once
#include "PhysicExtension/Controller/Controller.h"
#include "PhysicExtension/Utils/DroneDynamics.h"
#include "PhysicExtension/Utils/Wind.h"
#include "PhysicExtension/Vehicles/Multirotor.h"
#include "btBulletDynamicsCommon.h"
/**
 * @file Transform.h
 * @brief Transform component system for entity spatial information
 */

#pragma once
#include "ComponentCommon.h"

namespace lark::physics
{
/**
 * @struct init_info
 * @brief Initialization information for creating a physics component
 */
struct init_info
{
    drones::Multirotor vehicle;
};

/**
 * @brief Creates a new transform component for an entity
 * @param info Initialization information for the physics
 * @param entity The entity that will own this physics component
 * @return A new physics component instance
 */
component create(init_info info, game_entity::entity entity);

/**
 * @brief Removes a transform component
 * @param t The transform component to remove
 */
void remove(component t);

void shutdown();
} // namespace lark::physics
