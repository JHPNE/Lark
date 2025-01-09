#pragma once
#include "BaseComponent.h"

namespace lark::rotor {
    DEFINE_TYPED_ID(rotor_id);

    class drone_component final : public base_component<rotor_id> {
        using base_component<rotor_id>::base_component;
    };
};