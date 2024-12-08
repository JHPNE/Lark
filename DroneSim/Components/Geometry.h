#pragma once
#include "ComponentCommon.h"
#include "../Geometry/Geometry.h"

namespace drosim::geometry {

    struct init_info {
        tools::scene* scene{ nullptr };      ///< Scene containing the geometry data
        tools::mesh* mesh{ nullptr };        ///< Specific mesh to use
        tools::lod_group* lod_group{ nullptr }; ///< LOD group if using LOD
        bool is_dynamic{ false };            ///< Whether the geometry should be dynamic
    };

    component create(init_info info, game_entity::entity entity);
    void remove(geometry_id id);



}
