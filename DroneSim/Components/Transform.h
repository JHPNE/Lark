/**
 * @file Transform.h
 * @brief Transform component system for entity spatial information
 */

#pragma once
#include "ComponentCommon.h"

namespace drosim::transform {

    /**
     * @struct init_info
     * @brief Initialization information for creating a transform component
     */
    struct init_info {
        f32 position[3]{};                    ///< Initial position (x, y, z)
        f32 rotation[4]{};                    ///< Initial rotation as quaternion (x, y, z, w)
        f32 scale[3]{1.f, 1.f, 1.f};         ///< Initial scale (x, y, z)
    };

    /**
     * @brief Creates a new transform component for an entity
     * @param info Initialization information for the transform
     * @param entity The entity that will own this transform component
     * @return A new transform component instance
     */
    component create(init_info info, game_entity::entity entity);

    /**
     * @brief Removes a transform component
     * @param t The transform component to remove
     */
    void remove(component t);
}
