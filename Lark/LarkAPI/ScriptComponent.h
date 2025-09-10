#pragma once
#include "../Components/ComponentCommon.h"

namespace lark::script
{

DEFINE_TYPED_ID(script_id);

class component final
{
  public:
    // here {} instead of ()
    constexpr explicit component(script_id id) : _id{id} {};
    constexpr component() : _id{id::invalid_id} {};
    constexpr script_id get_id() const { return _id; }
    constexpr bool is_valid() const { return id::is_valid(_id); }

  private:
    script_id _id;
};

} // namespace lark::script
