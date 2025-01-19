#pragma once
#include "DroneCommonHeaders.h"

namespace lark {
  #define INIT_INFO(drone_component) namespace drone_component { struct init_info; }
  INIT_INFO(fuselage);
  INIT_INFO(battery);
  INIT_INFO(rotor);
  #undef INIT_INFO

  namespace drone_entity {
    struct entity_info {
      fuselage::init_info* fuselage{ nullptr };
      util::vector<battery::init_info*> batteries;
      util::vector<rotor::init_info*> rotors;

      bool is_valid_entity() const {
        return (fuselage != nullptr);
      }

    };

    entity create(entity_info info);
    void remove(drone_id id);
    bool is_alive(drone_id id);
    bool add_component(drone_id id, drone_data::BodyType component_type, const entity_info& info);
    void transform(drone_id id, glm::mat4& transform);
  }
}