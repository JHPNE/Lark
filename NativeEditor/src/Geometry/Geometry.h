#pragma once
#include "Asset.h"
#include <memory>
#include <vector>
#include <string>
#include "EngineAPI.h"

namespace drosim::editor {

class Mesh {
public:
    s32 vertex_size = 0;
    s32 vertex_count = 0;
    s32 index_size = 0;
    s32 index_count = 0;
    std::vector<u8> vertices;
    std::vector<u8> indices;
};

class MeshLOD {
public:
    std::string name;
    f32 lod_threshold = 0.0f;
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

    void FromRawData(const u8* data, size_t size) {
        if (!data || !size) {
            printf("[Geometry::FromRawData] Error: Invalid input - null data or zero size\n");
            return;
        }

        lod_groups.clear();

        try {
            size_t at{0};
            printf("[Geometry::FromRawData] Starting to process data of size: %zu\n", size);

            // Read LOD name length
            u32 lod_name_length{0};
            memcpy(&lod_name_length, &data[at], sizeof(u32));
            at += sizeof(u32);
            printf("[Geometry::FromRawData] LOD name length: %u\n", lod_name_length);

            // Skip LOD name
            at += lod_name_length;

            // Read number of LOD groups
            u32 lod_count{0};
            memcpy(&lod_count, &data[at], sizeof(u32));
            at += sizeof(u32);
            printf("[Geometry::FromRawData] LOD count: %u\n", lod_count);

            for(u32 lod_idx = 0; lod_idx < lod_count; ++lod_idx) {
                auto lod_group = std::make_shared<LODGroup>();

                // Read LOD group name length
                u32 group_name_length{0};
                memcpy(&group_name_length, &data[at], sizeof(u32));
                at += sizeof(u32);

                // Read LOD group name
                if (group_name_length) {
                    lod_group->name.resize(group_name_length);
                    memcpy(lod_group->name.data(), &data[at], group_name_length);
                    at += group_name_length;
                }

                // Read number of meshes in this LOD group
                u32 mesh_count{0};
                memcpy(&mesh_count, &data[at], sizeof(u32));
                at += sizeof(u32);
                printf("[Geometry::FromRawData] Mesh count in LOD %u: %u\n", lod_idx, mesh_count);

                // Process each mesh
                for(u32 mesh_idx = 0; mesh_idx < mesh_count; ++mesh_idx) {
                    auto mesh_lod = std::make_shared<MeshLOD>();
                    auto mesh = std::make_shared<Mesh>();

                    // Read mesh name length
                    u32 mesh_name_length{0};
                    memcpy(&mesh_name_length, &data[at], sizeof(u32));
                    at += sizeof(u32);

                    // Read mesh name
                    if (mesh_name_length) {
                        mesh_lod->name.resize(mesh_name_length);
                        memcpy(mesh_lod->name.data(), &data[at], mesh_name_length);
                        at += mesh_name_length;
                    }
                    printf("[Geometry::FromRawData] Processing mesh: %s\n", mesh_lod->name.c_str());

                    // Read mesh LOD ID
                    u32 lod_id{0};
                    memcpy(&lod_id, &data[at], sizeof(u32));
                    at += sizeof(u32);

                    // Read vertex data info
                    memcpy(&mesh->vertex_size, &data[at], sizeof(u32));
                    at += sizeof(u32);
                    memcpy(&mesh->vertex_count, &data[at], sizeof(u32));
                    at += sizeof(u32);

                    // Read index data info
                    memcpy(&mesh->index_size, &data[at], sizeof(u32));
                    at += sizeof(u32);
                    memcpy(&mesh->index_count, &data[at], sizeof(u32));
                    at += sizeof(u32);

                    printf("[Geometry::FromRawData] Mesh data info:\n");
                    printf("  Vertex - Size: %u, Count: %u\n", mesh->vertex_size, mesh->vertex_count);
                    printf("  Index  - Size: %u, Count: %u\n", mesh->index_size, mesh->index_count);

                    // Calculate buffer sizes
                    const size_t vertex_data_size = mesh->vertex_size * mesh->vertex_count;
                    const size_t index_data_size = mesh->index_size * mesh->index_count;

                    // Read vertex data
                    if (vertex_data_size) {
                        mesh->vertices.resize(vertex_data_size);
                        memcpy(mesh->vertices.data(), &data[at], vertex_data_size);
                        at += vertex_data_size;
                        printf("[Geometry::FromRawData] Read vertex data: %zu bytes\n", vertex_data_size);
                    }

                    // Read index data
                    if (index_data_size) {
                        mesh->indices.resize(index_data_size);
                        memcpy(mesh->indices.data(), &data[at], index_data_size);
                        at += index_data_size;
                        printf("[Geometry::FromRawData] Read index data: %zu bytes\n", index_data_size);
                    }

                    mesh_lod->meshes.push_back(mesh);
                    lod_group->lods.push_back(mesh_lod);
                }

                lod_groups.push_back(lod_group);
            }

        } catch (const std::exception& e) {
            printf("[Geometry::FromRawData] Exception caught: %s\n", e.what());
            lod_groups.clear();
        }
    }

    static std::unique_ptr<Geometry> LoadGeometry(const char* filename) {
        content_tools::SceneData scene_data;
        scene_data.import_settings.calculate_normals = 1;
        scene_data.import_settings.calculate_tangents = 1;
        if (!LoadObj(filename, &scene_data)) return nullptr;

        if (scene_data.buffer && scene_data.buffer_size > 0) {
            auto geometry = std::make_unique<Geometry>();
            geometry->FromRawData(scene_data.buffer, scene_data.buffer_size);
            free(scene_data.buffer);
            return geometry;
        }

        return nullptr;
    }

    static std::unique_ptr<Geometry> CreatePrimitive(
    content_tools::PrimitiveMeshType type,
    const f32* size = nullptr,
    const u32* segments = nullptr,
    u32 lod = 0
        ) {
        content_tools::PrimitiveInitInfo init_info{};
        init_info.type = type;

        // Fix the size assignment
        if (size) {
            init_info.size = drosim::math::v3(size[0], size[1], size[2]);
        }

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

} // namespace drosim::editor