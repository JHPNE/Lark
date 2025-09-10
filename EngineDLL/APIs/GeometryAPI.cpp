#include "GeometryAPI.h"

#define ENGINEDLL_EXPORTS

using namespace lark;

extern "C" {

    ENGINE_API bool CreatePrimitiveMesh(content_tools::SceneData* data,
                                      const content_tools::PrimitiveInitInfo* info) {
        if (!data || !info) {
            printf("[CreatePrimitiveMesh] Error: Invalid parameters\n");
            return false;
        }

        auto engine_data = lark::tools::scene_data{};
        auto engine_info = lark::tools::primitive_init_info{};

        // Convert info to engine format
        engine_info.mesh_type = static_cast<lark::tools::primitive_mesh_type>(info->type);
        memcpy(engine_info.segments, info->segments, sizeof(info->segments));
        engine_info.size = info->size;
        engine_info.lod = info->lod;

        engine_data.settings = data->import_settings;

        // Let the engine create the mesh
        printf("[CreatePrimitiveMesh] Calling engine CreatePrimitiveMesh\n");
        lark::api::create_primitive_mesh(&engine_data, &engine_info);

        // Just pass through the engine's buffer
        if (engine_data.buffer && engine_data.buffer_size > 0) {
            printf("[CreatePrimitiveMesh] Engine created buffer of size: %u\n", engine_data.buffer_size);
            data->buffer = engine_data.buffer;
            data->buffer_size = engine_data.buffer_size;
            return true;
        }

        printf("[CreatePrimitiveMesh] Failed to create buffer\n");
        return false;
    }

    ENGINE_API bool LoadObj(const char* path, content_tools::SceneData* data) {
        auto engine_data = lark::tools::scene_data{};
        engine_data.settings = data->import_settings;
        lark::api::load_geometry(path, &engine_data);

        if (engine_data.buffer && engine_data.buffer_size > 0) {
            data->buffer = engine_data.buffer;
            data->buffer_size = engine_data.buffer_size;
            return true;
        }

        return false;
    }

    ENGINE_API bool ModifyEntityVertexPositions(lark::id::id_type entity_id, std::vector<glm::vec3>& new_positions) {
        if(lark::api::update_dynamic_mesh(entity_id, new_positions)) {
            return true;
        };
        return false;
    }

    ENGINE_API bool GetModifiedMeshData(lark::id::id_type entity_id, content_tools::SceneData* data) {
        if (!data) return false;

        // Convert content tools scene data to engine scene data
        tools::scene_data engine_data{};

        // Get the mesh data from the entity
        bool success = lark::api::get_mesh_data(entity_id, &engine_data);
        if (!success) return false;

        // Copy the data to the output buffer
        data->buffer_size = engine_data.buffer_size;
        data->buffer = new u8[data->buffer_size];
        memcpy(data->buffer, engine_data.buffer, data->buffer_size);

        return true;
    }
}