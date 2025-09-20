#pragma once
#include "ComponentCommon.h"
#include "btBulletDynamicsCommon.h"
/**
 * @file Physics.h
 * @brief Phsyics component system for entity physics information
 */


namespace lark::physics
{
/**
 * @struct init_info
 * @brief Initialization information for creating a physics component
 */
struct init_info
{
    float mass{1.0f};
    math::v3 initial_position{0.0f};
    math::v4 initial_orientation{0.0f, 0.0f, 0.0f, 1.0f};
    math::v3 inertia{1.0f, 1.0f, 1.0f};
    std::shared_ptr<scene> scene{nullptr}; // For collision shape
    bool is_kinematic{false};};

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
