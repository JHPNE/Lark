#pragma once
#include "../DroneCommonHeaders.h"

namespace lark::fuselage {
  struct init_info : public lark::drone_data::FuselageBody{};

  drone_component create(init_info info, drone_entity::entity entity);

  void remove(drone_component t);
}