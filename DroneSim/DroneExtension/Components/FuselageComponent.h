#pragma once
#include "../DroneCommonHeaders.h"

namespace lark::fuselage {
  DEFINE_TYPED_ID(fuselage_id);

  class drone_component final {
    public:
      constexpr explicit drone_component(fuselage_id id) : _id{ id } {}
      constexpr drone_component() : _id{ id::invalid_id } {}
      constexpr fuselage_id get_id() const { return _id; }
      constexpr bool is_valid() const { return id::is_valid(_id); }

    private:
      fuselage_id _id;
  };
}