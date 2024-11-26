/**
 * @file Script.h
 * @brief Script component system for entity behavior control
 * 
 * This file defines the script component system that allows entities to have
 * programmable behaviors through Python scripts. It provides mechanisms for
 * creating, removing, and managing script components.
 */

#pragma once
#include "ComponentCommon.h"

namespace drosim::script {

    /**
     * @struct init_info
     * @brief Initialization information for creating a script component
     * 
     * Contains the necessary information to create and initialize a script
     * component, including the script creator function that will be used
     * to instantiate the actual script.
     */
    struct init_info {
        detail::script_creator script_creator;  ///< Function to create the script instance
    };

    /**
     * @brief Creates a new script component for an entity
     * @param info Initialization information for the script
     * @param entity The entity that will own this script component
     * @return A new script component instance
     * 
     * Creates and initializes a new script component using the provided
     * initialization information and attaches it to the specified entity.
     */
    component create(init_info info, game_entity::entity entity);

    /**
     * @brief Removes a script component
     * @param t The script component to remove
     * 
     * Properly cleans up the script component and removes it from its
     * associated entity. This includes cleaning up any Python resources.
     */
    void remove(component t);

    /**
     * @brief Shuts down the entire script component system
     * 
     * Cleans up all script components and releases all associated
     * resources. This should be called during engine shutdown.
     */
    void shutdown();
}