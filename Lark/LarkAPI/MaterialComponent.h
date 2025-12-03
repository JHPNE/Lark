#pragma once
#include "../Components/ComponentCommon.h"

namespace lark::material
{

DEFINE_TYPED_ID(material_id);

class component final
{
    public:
        constexpr explicit component(material_id id) : _id{id} {}
        constexpr component() : _id{id::invalid_id} {}
        constexpr material_id get_id() const { return _id; }
        constexpr bool is_valid() const { return id::is_valid(_id); }

    private:
        material_id _id;
};
}