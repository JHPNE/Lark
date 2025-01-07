// BaseComponent.h
#pragma once
#include "../DroneCommonHeaders.h"

namespace lark {
  template<typename IdType>
  class base_component {
  public:
    constexpr explicit base_component(IdType id) : _id{ id } {}
    constexpr base_component() : _id{ id::invalid_id } {}
    [[nodiscard]] constexpr IdType get_id() const { return _id; }
    [[nodiscard]] constexpr bool is_valid() const { return id::is_valid(_id); }

  private:
    IdType _id;
  };
}
