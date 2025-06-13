#pragma once
#include "ComponentCommon.h"
#include "../Geometry/Geometry.h"

namespace lark::geometry {
    struct init_info {
        tools::scene* scene{ nullptr };  ///< Scene containing the geometry data
        bool is_dynamic{ false };        ///< Whether the geometry should be dynamic
    };

    component create(init_info info, game_entity::entity entity);
    void remove(component t);
}
