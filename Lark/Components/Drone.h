#pragma once
#include "ComponentCommon.h"
#include "PhysicExtension/Controller/Controller.h"
#include "PhysicExtension/Utils/DroneDynamics.h"
#include "PhysicExtension/Vehicles/Multirotor.h"

/**
 * @file Physics.h
 * @brief Phsyics component system for entity physics information
 */

namespace lark::drone
{
    /**
     * @struct init_info
     * @brief Initialization information for creating a physics component
     */
    struct init_info
    {
        QuadParams params;
        ControlAbstraction abstraction;
        std::shared_ptr<Trajectory> trajectory{nullptr};
        DroneState initial_state;
        ControlInput last_control;
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
