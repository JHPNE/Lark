#pragma once
#include "../Geometry/MeshPrimitives.h"
#include "../Geometry/Geometry.h"
#include "../Geometry/GeometryImporter.h"

namespace lark::api {
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
        geometry.set_dynamic(true);

        if (geometry.is_dynamic()) {
            // First update vertices with new positions
            if (!geometry.update_vertices(new_positions)) return false;

            return true;
        }

        return false;
    }

    inline bool get_mesh_data(game_entity::entity_id id, tools::scene_data* data) {
        if (!id::is_valid(id) || !game_entity::is_alive(id) || !data) return false;
        
        game_entity::entity entity{ id };
        auto geometry = entity.geometry();
        if (!geometry.is_valid() || !geometry.is_dynamic()) return false;

        // Get the current mesh data
        auto* scene = geometry.get_scene();
        if (!scene) return false;

        // Pack the current mesh data into the output buffer
        tools::pack_data(*scene, *data);
        return (data->buffer && data->buffer_size > 0);
    }
}