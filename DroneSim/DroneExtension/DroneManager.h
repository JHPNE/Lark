#pragma once
#include "DroneCommonHeaders.h"

namespace lark {
  #define INIT_INFO(drone_component) namespace drone_component { struct init_info; }
  INIT_INFO(fuselage);
  INIT_INFO(battery);
  #undef INIT_INFO

  namespace drone_entity {
    struct entity_info {
      fuselage::init_info* fuselage{ nullptr };
      battery::init_info* battery{ nullptr };
    };

    enum class drone_components {
      FUSELAGE,
      ROTOR,
      WINGS,
    };

    entity create(entity_info info);

    void remove(drone_id id);

    bool is_alive(drone_id id);

    void addDroneComponent(drone_id id, drone_components component, entity_info info);
  }
}