#pragma once
#include "../Geometry/MeshPrimitives.h"
#include "../Geometry/Geometry.h"
#include "../Geometry/GeometryImporter.h"

namespace drosim::api {
    inline bool create_primitive_mesh(tools::scene_data* data, tools::primitive_init_info* info) {
        if (!data || !info) return false;
        CreatePrimitiveMesh(data, info);
        return (data->buffer && data->buffer_size > 0);
    }

    inline bool load_geometry(const char* path, tools::scene_data* data) {
        loadObj(path, data);
        return (data->buffer && data->buffer_size > 0);
    }

    inline bool update_dynamic_mesh(game_entity::entity_id id, const std::vector<math::v3>& new_positions) {
        if (!id::is_valid(id) || !game_entity::is_alive(id)) return false;
        
        game_entity::entity entity{ id };  // This is just a wrapper, not creating a new entity
        auto geometry = entity.geometry();
        if (!geometry.is_valid()) return false;

        // Get the geometry component and update if it's dynamic
        if (geometry.is_dynamic()) {
            // First update vertices with new positions
            if (!geometry.update_vertices(new_positions)) return false;
            
            // Then recalculate normals to update the buffer
            if (!geometry.recalculate_normals()) return false;
            
            return true;
        }

        return false;
    }
}