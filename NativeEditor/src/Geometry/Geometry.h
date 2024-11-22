#pragma once
#include "Asset.h"
#include <memory>
#include <vector>
#include <string>
#include "EngineAPI.h" // Only includes the public API interface

namespace editor {

class Mesh {
public:
    int32_t vertex_size = 0;
    int32_t vertex_count = 0;
    int32_t index_size = 0;
    int32_t index_count = 0;
    std::vector<uint8_t> vertices;
    std::vector<uint8_t> indices;
};

class MeshLOD {
public:
    std::string name;
    float lod_threshold = 0.0f;
    std::vector<std::shared_ptr<Mesh>> meshes;
};

class LODGroup {
public:
    std::string name;
    std::vector<std::shared_ptr<MeshLOD>> lods;
};

class Geometry : public Asset {
public:
    Geometry() : Asset(AssetType::Mesh) {}

    LODGroup* GetLODGroup(size_t index = 0) {
        if (index < lod_groups.size()) {
            return lod_groups[index].get();
        }
        return nullptr;
    }

    void FromRawData(const uint8_t* data, size_t size) {
        if (!data || !size) return;

        size_t at{ 0 };

        while (at < size) {
            // Read LOD group
            auto lod_group = std::make_shared<LODGroup>();

            // Read LOD name length
            uint32_t name_length{ 0 };
            memcpy(&name_length, &data[at], sizeof(uint32_t));
            at += sizeof(uint32_t);

            // Read LOD name
            if (name_length) {
                lod_group->name.resize(name_length);
                memcpy(lod_group->name.data(), &data[at], name_length);
                at += name_length;
            }

            // Read number of meshes in this LOD
            uint32_t mesh_count{ 0 };
            memcpy(&mesh_count, &data[at], sizeof(uint32_t));
            at += sizeof(uint32_t);

            // Read meshes
            for (uint32_t mesh_idx = 0; mesh_idx < mesh_count; ++mesh_idx) {
                auto mesh_lod = std::make_shared<MeshLOD>();
                auto mesh = std::make_shared<Mesh>();

                // Read mesh name length
                uint32_t mesh_name_length{ 0 };
                memcpy(&mesh_name_length, &data[at], sizeof(uint32_t));
                at += sizeof(uint32_t);

                // Read mesh name
                if (mesh_name_length) {
                    mesh_lod->name.resize(mesh_name_length);
                    memcpy(mesh_lod->name.data(), &data[at], mesh_name_length);
                    at += mesh_name_length;
                }

                // Read LOD ID (we skip it in editor)
                uint32_t lod_id;
                memcpy(&lod_id, &data[at], sizeof(uint32_t));
                at += sizeof(uint32_t);

                // Read vertex data
                memcpy(&mesh->vertex_size, &data[at], sizeof(uint32_t));
                at += sizeof(uint32_t);
                memcpy(&mesh->vertex_count, &data[at], sizeof(uint32_t));
                at += sizeof(uint32_t);

                // Read index data
                memcpy(&mesh->index_size, &data[at], sizeof(uint32_t));
                at += sizeof(uint32_t);
                memcpy(&mesh->index_count, &data[at], sizeof(uint32_t));
                at += sizeof(uint32_t);

                // Read vertex buffer
                const size_t vertex_buffer_size = mesh->vertex_size * mesh->vertex_count;
                if (vertex_buffer_size) {
                    mesh->vertices.resize(vertex_buffer_size);
                    memcpy(mesh->vertices.data(), &data[at], vertex_buffer_size);
                    at += vertex_buffer_size;
                }

                // Read index buffer
                const size_t index_buffer_size = mesh->index_size * mesh->index_count;
                if (index_buffer_size) {
                    mesh->indices.resize(index_buffer_size);
                    memcpy(mesh->indices.data(), &data[at], index_buffer_size);
                    at += index_buffer_size;
                }

                mesh_lod->meshes.push_back(mesh);
                lod_group->lods.push_back(mesh_lod);
            }

            lod_groups.push_back(lod_group);
        }
    }

    static std::unique_ptr<Geometry> CreatePrimitive(
        content_tools::PrimitiveMeshType type,
        const float* size = nullptr,
        const uint32_t* segments = nullptr,
        uint32_t lod = 0
    ) {
        content_tools::PrimitiveInitInfo init_info{};
        init_info.type = type;

        if (size) memcpy(init_info.size, size, sizeof(init_info.size));
        if (segments) memcpy(init_info.segments, segments, sizeof(init_info.segments));
        init_info.lod = lod;

        content_tools::SceneData scene_data{};
        scene_data.import_settings.calculate_normals = 1;
        scene_data.import_settings.calculate_tangents = 1;

        if (!CreatePrimitiveMesh(&scene_data, &init_info)) {
            return nullptr;
        }

        if (scene_data.buffer && scene_data.buffer_size > 0) {
            auto geometry = std::make_unique<Geometry>();
            geometry->FromRawData(scene_data.buffer, scene_data.buffer_size);
            free(scene_data.buffer);
            return geometry;
        }

        return nullptr;
    }

private:
    std::vector<std::shared_ptr<LODGroup>> lod_groups;
};

} // namespace editor