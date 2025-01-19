#pragma once
#include "Component.h"

namespace lark::battery {
  struct init_info : drone_data::BatteryBody {};

  drone_component create(init_info info, drone_entity::entity entity);

  void remove(drone_component t);
  void batteryCalculateCharge(drone_component t);
  glm::mat4 get_transform(drone_component t);
  void update_transform(drone_component t, glm::mat4& transform);
}