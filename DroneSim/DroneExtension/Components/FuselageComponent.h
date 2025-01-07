#pragma once
#include "BaseComponent.h"

namespace lark::fuselage {
  DEFINE_TYPED_ID(fuselage_id);

  class drone_component final : public base_component<fuselage_id> {
  public:
    using base_component<fuselage_id>::base_component;  // Inherit constructors
  };
}