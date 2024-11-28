#include "Geometry.h"
#include <execution>

namespace drosim::tools {
    namespace {

        inline math::v3 calculate_triangle_normal(const math::v3& v0, const math::v3& v1, const math::v3& v2) {
            const math::v3 e0{ v1 - v0 };
            const math::v3 e1{ v2 - v0 };
            return glm::normalize(glm::cross(e0, e1));
        }

        void recalculate_normals(mesh& m) {
            const u32 num_triangles = (u32)m.raw_indices.size() / 3;
            m.normals.resize(m.raw_indices.size());

            #pragma omp parallel for
            for (s32 i = 0; i < (s32)num_triangles; ++i) {
                const u32 base_idx = i * 3;
                const u32 i0 = m.raw_indices[base_idx];
                const u32 i1 = m.raw_indices[base_idx + 1];
                const u32 i2 = m.raw_indices[base_idx + 2];

                const math::v3 n = calculate_triangle_normal(
                    m.positions[i0],
                    m.positions[i1],
                    m.positions[i2]
                );

                // Store normal for all three vertices
                m.normals[base_idx] = n;
                m.normals[base_idx + 1] = n;
                m.normals[base_idx + 2] = n;
            }
        }

        void process_normals(mesh& m, f32 smoothing_angle) {
            const float cos_alpha = glm::cos(glm::pi<float>() - smoothing_angle * glm::pi<float>() / 180.f);
            const bool is_hard_edge = glm::epsilonEqual(smoothing_angle, 180.f, math::epsilon);
            const bool is_soft_edge = glm::epsilonEqual(smoothing_angle, 0.f, math::epsilon);

            const u32 num_indices = (u32)m.raw_indices.size();
            const u32 num_vertices = (u32)m.positions.size();
            assert(num_indices && num_vertices);

            m.indices.resize(num_indices);
            m.vertices.reserve(num_vertices); // Pre-allocate to avoid reallocations

            // Create index reference for vertices
            std::vector<std::vector<u32>> idx_ref(num_vertices);

            // Pre-calculate approximate size for index references
            const u32 avg_refs_per_vertex = (num_indices + num_vertices - 1) / num_vertices;
            for (auto& refs : idx_ref) {
                refs.reserve(avg_refs_per_vertex);
            }

            // Build index references in parallel
            #pragma omp parallel for
            for (s32 i = 0; i < (s32)num_indices; ++i) {
                const u32 vertex_idx = m.raw_indices[i];
                #pragma omp critical
                {
                    idx_ref[vertex_idx].push_back(i);
                }
            }

            // Process vertices
            #pragma omp parallel for
            for (s32 i = 0; i < (s32)num_vertices; ++i) {
                auto& refs = idx_ref[i];
                u32 num_refs = (u32)refs.size();

                for (u32 j = 0; j < num_refs; ++j) {
                    vertex v;
                    v.position = m.positions[m.raw_indices[refs[j]]];
                    v.normal = m.normals[refs[j]];

                    u32 vertex_index;
                    #pragma omp critical
                    {
                        vertex_index = (u32)m.vertices.size();
                        m.vertices.push_back(v);
                    }
                    m.indices[refs[j]] = vertex_index;

                    if (!is_hard_edge) {
                        for (u32 k = j + 1; k < num_refs; ++k) {
                            float cos_theta = 0.f;
                            const math::v3& n2 = m.normals[refs[k]];

                            if (!is_soft_edge) {
                                cos_theta = glm::dot(v.normal, n2) * glm::length(v.normal);
                            }

                            if (is_soft_edge || cos_theta >= cos_alpha) {
                                v.normal += n2;
                                m.indices[refs[k]] = vertex_index;
                                refs.erase(refs.begin() + k);
                                --num_refs;
                                --k;
                            }
                        }
                    }
                }
            }
        }

        void process_uvs(mesh& m) {
            if (m.uv_sets.empty()) return;

            const u32 num_vertices = (u32)m.vertices.size();
            const u32 num_indices = (u32)m.indices.size();
            if (!num_vertices || !num_indices) return;

            util::vector<vertex> new_vertices;
            new_vertices.reserve(num_vertices * 2); // Estimate for split vertices
            util::vector<u32> new_indices(num_indices);

            // Create lookup table for vertex-UV pairs
            struct VertexUVPair {
                u32 vertex_index;
                math::v2 uv;
                u32 new_index;
            };
            std::vector<VertexUVPair> vertex_uv_pairs;
            vertex_uv_pairs.reserve(num_indices);

            // Collect vertex-UV pairs
            for (u32 i = 0; i < num_indices; ++i) {
                vertex_uv_pairs.push_back({
                    m.indices[i],
                    m.uv_sets[0][i],
                    u32_invalid_id
                });
            }

            // Sort pairs for faster lookup
            std::sort(std::execution::par_unseq, vertex_uv_pairs.begin(), vertex_uv_pairs.end(),
                [](const VertexUVPair& a, const VertexUVPair& b) {
                    if (a.vertex_index != b.vertex_index) return a.vertex_index < b.vertex_index;
                    if (a.uv.x != b.uv.x) return a.uv.x < b.uv.x;
                    return a.uv.y < b.uv.y;
                });

            // Process unique vertex-UV combinations
            for (size_t i = 0; i < vertex_uv_pairs.size(); ++i) {
                if (vertex_uv_pairs[i].new_index != u32_invalid_id) continue;

                vertex v = m.vertices[vertex_uv_pairs[i].vertex_index];
                v.uv = vertex_uv_pairs[i].uv;
                vertex_uv_pairs[i].new_index = (u32)new_vertices.size();
                new_vertices.push_back(v);

                // Find matching pairs
                for (size_t j = i + 1; j < vertex_uv_pairs.size(); ++j) {
                    if (vertex_uv_pairs[j].vertex_index != vertex_uv_pairs[i].vertex_index) break;

                    if (glm::epsilonEqual(vertex_uv_pairs[j].uv.x, vertex_uv_pairs[i].uv.x, math::epsilon) &&
                        glm::epsilonEqual(vertex_uv_pairs[j].uv.y, vertex_uv_pairs[i].uv.y, math::epsilon)) {
                        vertex_uv_pairs[j].new_index = vertex_uv_pairs[i].new_index;
                    }
                }
            }

            // Update indices
            #pragma omp parallel for
            for (s32 i = 0; i < (s32)num_indices; ++i) {
                new_indices[i] = vertex_uv_pairs[i].new_index;
            }

            m.vertices = std::move(new_vertices);
            m.indices = std::move(new_indices);
        }

        void pack_vertices_static(mesh& m) {
            const u32 num_vertices = (u32)m.vertices.size();
            if (!num_vertices) return;

            m.packed_vertices_static.resize(num_vertices);

            #pragma omp parallel for
            for (s32 i = 0; i < (s32)num_vertices; ++i) {
                const vertex& v = m.vertices[i];
                const u8 signs = (u8)((v.normal.z > 0.f) << 1);
                const u16 normal_x = (u16)math::pack_float<16>(v.normal.x, -1.f, 1.f);
                const u16 normal_y = (u16)math::pack_float<16>(v.normal.y, -1.f, 1.f);

                m.packed_vertices_static[i] = packed_vertex::vertex_static{
                    v.position,
                    {0, 0, 0},
                    signs,
                    normal_x,
                    normal_y,
                    {},
                    v.uv
                };
            }
        }

        void process_vertices(mesh& m, const geometry_import_settings& settings) {
            assert((m.raw_indices.size() % 3) == 0);
            if (settings.calculate_normals || m.normals.empty()) {
                recalculate_normals(m);
            }

            process_normals(m, settings.smoothing_angle);

            if (!m.uv_sets.empty()) {
                process_uvs(m);
            }

            pack_vertices_static(m);
        }

        u64 get_mesh_size(const mesh& m) {
            const u64 num_vertices{ m.vertices.size() };
            const u64 vertex_buffer_size{ sizeof(packed_vertex::vertex_static) * num_vertices };
            const u64 index_size{ (num_vertices < (1 << 16)) ? sizeof(u16) : sizeof(u32) };
            const u64 index_buffer_size{ index_size * m.indices.size() };
            constexpr u64 su32{ sizeof(u32) };

            const u64 size{
                su32 + m.name.size() +  // mesh name length and string
                su32 +                  // mesh id
                su32 + su32 +          // vertex size, number of vertices
                su32 + su32 +          // index size, number of indices
                sizeof(f32) +          // lod threshold
                vertex_buffer_size +    // vertices
                index_buffer_size       // indices
            };

            return size;
        }

        u64 get_scene_size(const scene& scene) {
            constexpr u64 su32{ sizeof(u32) };
            u64 size{ su32 + scene.name.size() + su32 };  // name length, name string, number of LODs

            for (const auto& lod : scene.lod_groups) {
                u64 lod_size{ su32 + lod.name.size() + su32 };  // LOD name length, name string, number of meshes

                for (const auto& m : lod.meshes) {
                    lod_size += get_mesh_size(m);
                }
                size += lod_size;
            }

            return size;
        }

        void pack_mesh_data(const mesh& m, u8* const buffer, u64& at) {
            constexpr u64 su32{ sizeof(u32) };
            u32 s{ 0 };

            // Write mesh name
            s = (u32)m.name.size();
            memcpy(&buffer[at], &s, su32);
            at += su32;
            memcpy(&buffer[at], m.name.c_str(), s);
            at += s;

            // Write mesh id
            s = m.lod_id;
            memcpy(&buffer[at], &s, su32);
            at += su32;

            // Write vertex info
            constexpr u32 vertex_size{ sizeof(packed_vertex::vertex_static) };
            s = vertex_size;
            memcpy(&buffer[at], &s, su32);
            at += su32;

            const u32 num_vertices{ (u32)m.vertices.size() };
            s = num_vertices;
            memcpy(&buffer[at], &s, su32);
            at += su32;

            // Write index info
            const size_t index_size{ (num_vertices < (1 << 16)) ? sizeof(u16) : sizeof(u32) };
            s = (u32)index_size;
            memcpy(&buffer[at], &s, su32);
            at += su32;

            const u32 num_indices{ (u32)m.indices.size() };
            s = num_indices;
            memcpy(&buffer[at], &s, su32);
            at += su32;

            // Write vertex data
            s = vertex_size * num_vertices;
            memcpy(&buffer[at], m.packed_vertices_static.data(), s);
            at += s;

            // Write index data
            s = (u32)(index_size * num_indices);
            void* data{ (void*)m.indices.data() };
            util::vector<u16> indices;

            if (index_size == sizeof(u16)) {
                indices.resize(num_indices);
                for (u32 i{ 0 }; i < num_indices; ++i) {
                    indices[i] = (u16)m.indices[i];
                }
                data = (void*)indices.data();
            }

            memcpy(&buffer[at], data, s);
            at += s;
        }
    }

    void process_scene(scene& scene, const geometry_import_settings& settings) {
        // Calculate total number of meshes for better load balancing
        size_t total_meshes = 0;
        for (const auto& lod : scene.lod_groups) {
            total_meshes += lod.meshes.size();
        }

        // Create a flat vector of mesh pointers for parallel processing
        std::vector<mesh*> all_meshes;
        all_meshes.reserve(total_meshes);

        for (auto& lod : scene.lod_groups) {
            for (auto& m : lod.meshes) {
                all_meshes.push_back(&m);
            }
        }

        // Process all meshes in parallel
        #pragma omp parallel for schedule(dynamic)
        for (s32 i = 0; i < (s32)all_meshes.size(); ++i) {
            process_vertices(*all_meshes[i], settings);
        }
    }

    void pack_data(const scene& scene, scene_data& data) {
        constexpr u64 su32{ sizeof(u32) };
        const u64 scene_size{ get_scene_size(scene) };
        data.buffer_size = (u32)scene_size;
        data.buffer = (u8*)malloc(scene_size);
        assert(data.buffer);

        u8* const buffer{ data.buffer };
        u64 at{ 0 };
        u32 s{ 0 };

        // Write scene name
        s = (u32)scene.name.size();
        memcpy(&buffer[at], &s, su32);
        at += su32;
        memcpy(&buffer[at], scene.name.c_str(), s);
        at += s;

        // Write number of LODs
        s = (u32)scene.lod_groups.size();
        memcpy(&buffer[at], &s, su32);
        at += su32;

        // Write LOD groups
        for (auto& lod : scene.lod_groups) {
            // Write LOD name
            s = (u32)lod.name.size();
            memcpy(&buffer[at], &s, su32);
            at += su32;
            memcpy(&buffer[at], lod.name.c_str(), s);
            at += s;

            // Write number of meshes
            s = (u32)lod.meshes.size();
            memcpy(&buffer[at], &s, su32);
            at += su32;

            // Write meshes
            for (auto& m : lod.meshes) {
                pack_mesh_data(m, buffer, at);
            }
        }
    }
}