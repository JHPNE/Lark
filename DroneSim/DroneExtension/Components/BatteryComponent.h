#pragma once
#include "BaseComponent.h"

namespace lark::battery {
  DEFINE_TYPED_ID(battery_id);

  class drone_component final : public lark::base_component<battery_id> {
  public:
    using base_component<battery_id>::base_component;  // Inherit constructors
  };
}