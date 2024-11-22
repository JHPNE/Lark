#pragma once
#include "../Geometry/MeshPrimitives.h"
#include "../Geometry/Geometry.h"

namespace drosim::api {
    inline bool create_primitive_mesh(tools::scene_data* data, tools::primitive_init_info* info) {
        if (!data || !info) return false;
        tools::CreatePrimitiveMesh(data, info);
        return (data->buffer && data->buffer_size > 0);
    }
}