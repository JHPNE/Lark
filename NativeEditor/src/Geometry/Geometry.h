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

    drosim::editor::scene* GetScene(size_t index = 0) {
        if (index < scenes.size()) {
            return &scenes[index];
        }
        return nullptr;
    }

    bool FromRawData(const u8* data, size_t size) {
        if (!data || !size) return false;

        scenes.clear();

        size_t at = 0;
        constexpr size_t su32 = sizeof(u32);

        // Read scene name length
        u32 scene_name_length = 0;
        memcpy(&scene_name_length, &data[at], su32);
        at += su32;

        // Read scene name
        std::string scene_name(scene_name_length, '\0');
        memcpy(scene_name.data(), &data[at], scene_name_length);
        at += scene_name_length;

        // Read number of LOD groups
        u32 lod_count{0};
        memcpy(&lod_count, &data[at], su32);
        at += su32;

        printf("[Geometry::FromRawData] Scene: %s, LOD count: %u\n", scene_name.c_str(), lod_count);

        drosim::editor::scene main_scene;
        main_scene.name = scene_name;

        for (u32 lod_idx = 0; lod_idx < lod_count; ++lod_idx) {
            drosim::editor::lod_group lod_group;

            // Read LOD group name length
            u32 group_name_length{0};
            memcpy(&group_name_length, &data[at], su32);
            at += su32;

            std::string lod_name;
            if (group_name_length) {
                lod_name.resize(group_name_length);
                memcpy(lod_name.data(), &data[at], group_name_length);
                at += group_name_length;
            }
            lod_group.name = lod_name;

            // Read number of meshes in this LOD group
            u32 mesh_count{0};
            memcpy(&mesh_count, &data[at], su32);
            at += su32;

            printf("[Geometry::FromRawData] LOD Group '%s' has %u meshes\n", lod_name.c_str(), mesh_count);

            for (u32 mesh_idx = 0; mesh_idx < mesh_count; ++mesh_idx) {
                drosim::editor::mesh mesh;

                // Mesh name length
                u32 mesh_name_length{0};
                memcpy(&mesh_name_length, &data[at], su32);
                at += su32;

                std::string mesh_name;
                if (mesh_name_length) {
                    mesh_name.resize(mesh_name_length);
                    memcpy(mesh_name.data(), &data[at], mesh_name_length);
                    at += mesh_name_length;
                }
                mesh.name = mesh_name;

                printf("[Geometry::FromRawData] Processing mesh: %s\n", mesh.name.c_str());

                // Mesh LOD ID
                u32 lod_id{0};
                memcpy(&lod_id, &data[at], su32);
                at += su32;
                mesh.lod_id = lod_id;

                // Vertex size
                u32 vertex_size = 0;
                memcpy(&vertex_size, &data[at], su32);
                at += su32;

                // Vertex count
                u32 vertex_count{0};
                memcpy(&vertex_count, &data[at], su32);
                at += su32;

                // Index size
                u32 idx_size = 0;
                memcpy(&idx_size, &data[at], su32);
                at += su32;

                // Index count
                u32 index_count{0};
                memcpy(&index_count, &data[at], su32);
                at += su32;

                // LOD threshold
                f32 lod_threshold = 0.0f;
                memcpy(&lod_threshold, &data[at], sizeof(f32));
                at += sizeof(f32);
                mesh.lod_threshold = lod_threshold;

                printf("[Geometry::FromRawData] Mesh '%s': vertex_count=%u, index_count=%u, lod_threshold=%.2f\n",
                       mesh.name.c_str(), vertex_count, index_count, lod_threshold);

                // Read vertices
                if (vertex_count > 0) {
                    std::vector<drosim::tools::packed_vertex::vertex_static> packed_verts(vertex_count);
                    memcpy(packed_verts.data(), &data[at], vertex_count * vertex_size);
                    at += vertex_count * vertex_size;

                    mesh.vertices.resize(vertex_count);
                    mesh.positions.resize(vertex_count);
                    mesh.normals.resize(vertex_count);
                    mesh.tangents.resize(vertex_count);
                    mesh.uv_sets.resize(1);
                    mesh.uv_sets[0].resize(vertex_count);

                    for (u32 i = 0; i < vertex_count; ++i) {
                        const auto& pv = packed_verts[i];

                        float nx = math::unpack_float<16>(pv.normal[0], -1.0f, 1.0f);
                        float ny = math::unpack_float<16>(pv.normal[1], -1.0f, 1.0f);

                        float length_sq = nx*nx + ny*ny;
                        float nz = (length_sq < 1.0f) ? sqrtf(1.0f - length_sq) : 0.0f;
                        if ((pv.t_sign & 0x02) == 0) {
                            nz = -nz;
                        }

                        mesh.positions[i] = pv.position;
                        mesh.normals[i]   = math::v3(nx, ny, nz);
                        mesh.uv_sets[0][i] = pv.uv;

                        // Default tangent
                        mesh.tangents[i] = math::v4(1.0f, 0.0f, 0.0f, 1.0f);

                        mesh.vertices[i].position = mesh.positions[i];
                        mesh.vertices[i].normal   = mesh.normals[i];
                        mesh.vertices[i].tangent  = mesh.tangents[i];
                        mesh.vertices[i].uv       = mesh.uv_sets[0][i];
                    }

                    // Print first few vertices for debugging
                    for (u32 i = 0; i < std::min(vertex_count, 5u); ++i) {
                        printf("[Debug] Vert %u: Pos(%.3f, %.3f, %.3f), Normal(%.3f, %.3f, %.3f), UV(%.3f, %.3f)\n",
                               i,
                               mesh.vertices[i].position.x, mesh.vertices[i].position.y, mesh.vertices[i].position.z,
                               mesh.vertices[i].normal.x, mesh.vertices[i].normal.y, mesh.vertices[i].normal.z,
                               mesh.vertices[i].uv.x, mesh.vertices[i].uv.y);
                    }
                }

                // Read indices
                if (index_count > 0) {
                    mesh.indices.resize(index_count);
                    if (idx_size == sizeof(u32)) {
                        memcpy(mesh.indices.data(), &data[at], index_count * sizeof(u32));
                        at += index_count * sizeof(u32);
                    } else if (idx_size == sizeof(u16)) {
                        std::vector<u16> temp_indices(index_count);
                        memcpy(temp_indices.data(), &data[at], index_count * sizeof(u16));
                        at += index_count * sizeof(u16);

                        for (u32 i = 0; i < index_count; ++i) {
                            mesh.indices[i] = (u32)temp_indices[i];
                        }
                    } else {
                        printf("[Geometry::FromRawData] Unexpected index size: %u\n", idx_size);
                        return false;
                    }

                    // Print first few indices for debugging
                    for (u32 i = 0; i < std::min(index_count, 15u); i += 3) {
                        printf("[Debug] Indices %u-%u: %u, %u, %u\n", i, i+2,
                               mesh.indices[i],
                               (i+1 < index_count ? mesh.indices[i+1] : 0),
                               (i+2 < index_count ? mesh.indices[i+2] : 0));
                    }
                }

                lod_group.meshes.push_back(std::move(mesh));
            }

            main_scene.lod_groups.push_back(std::move(lod_group));
        }

        scenes.push_back(std::move(main_scene));
        printf("[Geometry::FromRawData] Finished Processing\n");

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

    static void modifyVertexes(uint32_t id, std::vector<glm::vec3>& vertices) {
        ModifyEntityVertexPositions(id, vertices);
    }

    static void randomModificationVertexes(uint32_t id, uint32_t vertexCount, std::vector<glm::vec3> old_vertices) {
        std::vector<glm::vec3> vertices(vertexCount);
        std::srand(std::time(nullptr));
        for (u32 i = 0; i < vertexCount; i++) {
            /*
            // Get angle based on vertex index
            float angle = static_cast<float>(i) * 0.1f;
            
            // Create a spiral-like pattern
            vertices[i].x = 3.1f * sinf(angle);
            vertices[i].y = 2.1f * cosf(angle);
            vertices[i].z = 1.05f * sinf(angle * 2.0f);
            */

            vertices[i].x = old_vertices[i].x + ((float)rand() / (float)RAND_MAX) * 1;
            vertices[i].y = old_vertices[i].y +((float)rand() / (float)RAND_MAX) * 1;
            vertices[i].z = old_vertices[i].z +((float)rand() / (float)RAND_MAX) * 1;
        }

        modifyVertexes(id, vertices);
    }
private:
    std::vector<drosim::editor::scene> scenes;
};

} // namespace drosim::editor