#pragma once
#include "Component.h"
#include "Models/ISA.h"
#include "Models/Turbulence.h"
#include "Models/PropWash.h"

namespace lark::rotor {
  struct init_info : drone_data::RotorBody{};

  drone_component create(init_info info, drone_entity::entity entity);
  void remove(drone_component t);
  glm::mat4 get_transform(drone_component t);
  void update_transform(drone_component t, glm::mat4& transform);
}