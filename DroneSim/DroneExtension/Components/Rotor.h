#pragma once
#include "Component.h"

namespace lark::rotor {
  struct init_info : drone_data::RotorBody{};

  drone_component create(init_info info, drone_entity::entity entity);
  void remove(drone_component t);
  glm::mat4 get_transform(drone_component t);
  void update_transform(drone_component t, glm::mat4& transform);
}