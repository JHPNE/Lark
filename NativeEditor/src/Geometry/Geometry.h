#pragma once
#include "Asset.h"
#include <cstdlib>
#include <memory>
#include <vector>
#include <string>
#include <glm/gtc/random.hpp>
#include "EngineAPI.h"

namespace drosim::editor {
class Geometry : public Asset {
public:
    Geometry() : Asset(AssetType::Mesh) {}

    drosim::tools::scene* GetScene(size_t index = 0) {
        if (index < scenes.size()) {
            return &scenes[index];
        }
        return nullptr;
    }

    bool FromRawData(const u8* data, size_t size) {
        if (!data || !size) return false;

        scenes.clear();

        size_t at{0};

        // Read number of LOD groups
        u32 lod_count{0};
        memcpy(&lod_count, &data[at], sizeof(u32));
        at += sizeof(u32);

        printf("[Geometry::FromRawData] LOD count: %u\n", lod_count);

        for(u32 lod_idx = 0; lod_idx < lod_count; ++lod_idx) {
            drosim::tools::scene scene;
            drosim::tools::lod_group lod_group;

            // Read LOD group name length
            u32 group_name_length{0};
            memcpy(&group_name_length, &data[at], sizeof(u32));
            at += sizeof(u32);

            // Read LOD group name
            if (group_name_length) {
                lod_group.name.resize(group_name_length);
                memcpy(lod_group.name.data(), &data[at], group_name_length);
                at += group_name_length;
            }

            // Read number of meshes in this LOD group
            u32 mesh_count{0};
            memcpy(&mesh_count, &data[at], sizeof(u32));
            at += sizeof(u32);

            // Process each mesh
            for(u32 mesh_idx = 0; mesh_idx < mesh_count; ++mesh_idx) {
                drosim::tools::mesh mesh;

                // Read mesh name length
                u32 mesh_name_length{0};
                memcpy(&mesh_name_length, &data[at], sizeof(u32));
                at += sizeof(u32);

                // Read mesh name
                if (mesh_name_length) {
                    mesh.name.resize(mesh_name_length);
                    memcpy(mesh.name.data(), &data[at], mesh_name_length);
                    at += mesh_name_length;
                }
                printf("[Geometry::FromRawData] Processing mesh: %s\n", mesh.name.c_str());

                // Read mesh LOD ID
                u32 lod_id{0};
                memcpy(&lod_id, &data[at], sizeof(u32));
                mesh.lod_id = lod_id;
                at += sizeof(u32);

                // Read vertex data
                u32 vertex_count{0};
                memcpy(&vertex_count, &data[at], sizeof(u32));
                at += sizeof(u32);

                if (vertex_count > 0) {
                    mesh.positions.resize(vertex_count);
                    mesh.normals.resize(vertex_count);
                    mesh.tangents.resize(vertex_count);
                    mesh.uv_sets.resize(1);  // Assuming 1 UV set for now
                    mesh.uv_sets[0].resize(vertex_count);

                    // Read vertex data
                    const size_t vertex_data_size = sizeof(drosim::tools::vertex) * vertex_count;
                    std::vector<drosim::tools::vertex> vertices(vertex_count);
                    memcpy(vertices.data(), &data[at], vertex_data_size);
                    at += vertex_data_size;

                    // Populate mesh data
                    for (u32 i = 0; i < vertex_count; ++i) {
                        mesh.positions[i] = vertices[i].position;
                        mesh.normals[i] = vertices[i].normal;
                        mesh.tangents[i] = vertices[i].tangent;
                        mesh.uv_sets[0][i] = vertices[i].uv;
                    }
                }

                // Read index data
                u32 index_count{0};
                memcpy(&index_count, &data[at], sizeof(u32));
                at += sizeof(u32);

                if (index_count > 0) {
                    mesh.indices.resize(index_count);
                    const size_t index_data_size = sizeof(u32) * index_count;
                    memcpy(mesh.indices.data(), &data[at], index_data_size);
                    at += index_data_size;
                }

                lod_group.meshes.push_back(std::move(mesh));
            }

            scene.lod_groups.push_back(std::move(lod_group));
            scenes.push_back(std::move(scene));
        }

        return true;
    }

    bool ToRawData(std::vector<u8>& data) const {
        // Implementation for serialization
        // TODO: Implement this based on new structure
        return false;
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

    void modifyVertexes(uint32_t id, std::vector<glm::vec3>& vertices) {
        ModifyEntityVertexPositions(id, vertices);
    }

    void randomModificationVertexes(uint32_t id, uint32_t vertexCount) {
        std::vector<glm::vec3> vertices(vertexCount);
        for (u32 i = 0; i < vertexCount; i++) {
             vertices[i] = glm::linearRand(glm::vec3(-1.0f), glm::vec3(1.0f));
        }

        modifyVertexes(id, vertices);
    }

private:
    std::vector<drosim::tools::scene> scenes;
};

} // namespace drosim::editor