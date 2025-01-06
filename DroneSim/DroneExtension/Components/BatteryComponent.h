#pragma once
#include "../DroneCommonHeaders.h"

namespace lark::battery {
  DEFINE_TYPED_ID(battery_id);

  class drone_component final {
    public:
      constexpr explicit drone_component(battery_id id) : _id{ id } {}
      constexpr drone_component() : _id{ id::invalid_id } {}
      constexpr battery_id get_id() const { return _id; }
      constexpr bool is_valid() const { return id::is_valid(_id); }

    private:
      battery_id _id;
  };
}