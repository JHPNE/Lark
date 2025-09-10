#pragma once
#include "../Geometry/Geometry.h"
#include "ComponentCommon.h"

namespace lark::geometry
{
struct init_info
{
    std::shared_ptr<tools::scene> scene{nullptr}; ///< Scene containing the geometry data
    bool is_dynamic{false};                       ///< Whether the geometry should be dynamic
};

component create(init_info info, game_entity::entity entity);
void remove(component t);
} // namespace lark::geometry
