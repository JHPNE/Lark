/**
* @file Transform.h
 * @brief Transform component system for entity spatial information
 */

#pragma once
#include "ComponentCommon.h"
#include "Physics/DroneTypes.h"
#include "Physics/Wind.h"

namespace lark::physics {

    /**
     * @struct init_info
     * @brief Initialization information for creating a physics component
     */
    struct init_info {
        drones::InertiaProperties inertia;
        drones::AerodynamicProperties aerodynamic;
        drones::MotorProperties motor;
        std::vector<drones::RotorParameters> rotors;
        drones::ControlMode control_mode{drones::ControlMode::MOTOR_SPEEDS};
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
}
