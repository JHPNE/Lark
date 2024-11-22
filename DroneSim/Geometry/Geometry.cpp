#include "Geometry.h"

namespace drosim::tools {
    namespace {
        void recalculate_normals(mesh& m) {
            const u32 num_indices{ (u32)m.raw_indices.size() };
            m.normals.resize(m.raw_indices.size());

            for (u32 i = 0; i < num_indices; i += 3) {
                const u32 i0{ m.raw_indices[i] };
                const u32 i1{ m.raw_indices[i + 1] };
                const u32 i2{ m.raw_indices[i + 2] };

                math::v3 v0{ m.positions[i0] };
                math::v3 v1{ m.positions[i1] };
                math::v3 v2{ m.positions[i2] };

                math::v3 e0{ v1 - v0 };
                math::v3 e1{ v2 - v0 };
                math::v3 n{ glm::normalize(glm::cross(e0, e1)) };

                m.normals[i] = n;
                m.normals[i + 1] = n;
                m.normals[i + 2] = n;
            }
        }

        void process_normals(mesh& m, f32 smoothing_angle) {
            const float cos_alpha{ glm::cos(glm::pi<float>() - smoothing_angle * glm::pi<float>() / 180.f) };
            const bool is_hard_edge{ glm::epsilonEqual(smoothing_angle, 180.f, math::epsilon) };
            const bool is_soft_edge{ glm::epsilonEqual(smoothing_angle, 0.f, math::epsilon) };

            const u32 num_indices{ (u32)m.raw_indices.size() };
            const u32 num_vertices{ (u32)m.positions.size() };
            assert(num_indices && num_vertices);

            m.indices.resize(num_indices);

            // Create index reference for vertices
            util::vector<util::vector<u32>> idx_ref(num_vertices);
            for (u32 i = 0; i < num_indices; ++i) {
                idx_ref[m.raw_indices[i]].emplace_back(i);
            }

            for (u32 i = 0; i < num_vertices; ++i) {
                auto& refs{ idx_ref[i] };
                u32 num_refs{ (u32)refs.size() };

                for (u32 j = 0; j < num_refs; ++j) {
                    m.indices[refs[j]] = (u32)m.vertices.size();
                    vertex& v{ m.vertices.emplace_back() };
                    v.position = m.positions[m.raw_indices[refs[j]]];

                    math::v3 n1{ m.normals[refs[j]] };
                    if (!is_hard_edge) {
                        for (u32 k = j + 1; k < num_refs; ++k) {
                            float cos_theta{ 0.f };
                            math::v3 n2{ m.normals[refs[k]] };
                            if (!is_soft_edge) {
                                cos_theta = glm::dot(n1, n2) * glm::length(n1);
                            }

                            if (is_soft_edge || cos_theta >= cos_alpha) {
                                n1 += n2;
                                m.indices[refs[k]] = m.indices[refs[j]];
                                refs.erase(refs.begin() + k);
                                --num_refs;
                                --k;
                            }
                        }
                    }
                    v.normal = glm::normalize(n1);
                }
            }
        }

        void process_uvs(mesh& m) {
            if (m.uv_sets.empty()) return;

            util::vector<vertex> old_vertices;
            old_vertices.swap(m.vertices);
            util::vector<u32> old_indices(m.indices.size());
            old_indices.swap(m.indices);

            const u32 num_vertices{ (u32)old_vertices.size() };
            const u32 num_indices{ (u32)old_indices.size() };
            assert(num_vertices && num_indices);

            util::vector<util::vector<u32>> idx_ref(num_vertices);
            for (u32 i{ 0 }; i < num_indices; ++i) {
                idx_ref[old_indices[i]].emplace_back(i);
            }

            for (u32 i{ 0 }; i < num_vertices; ++i) {
                auto& refs{ idx_ref[i] };
                u32 num_refs{ (u32)refs.size() };
                for (u32 j{ 0 }; j < num_refs; ++j) {
                    m.indices[refs[j]] = (u32)m.vertices.size();
                    vertex& v{ old_vertices[old_indices[refs[j]]] };
                    v.uv = m.uv_sets[0][refs[j]];
                    m.vertices.emplace_back(v);

                    for (u32 k{ j + 1 }; k < num_refs; ++k) {
                        math::v2& uv1{ m.uv_sets[0][refs[k]] };
                        if (glm::epsilonEqual(v.uv.x, uv1.x, math::epsilon) &&
                            glm::epsilonEqual(v.uv.y, uv1.y, math::epsilon))
                        {
                            m.indices[refs[k]] = m.indices[refs[j]];
                            refs.erase(refs.begin() + k);
                            --num_refs;
                            --k;
                        }
                    }
                }
            }
        }

        void pack_vertices_static(mesh& m) {
            const u32 num_vertices{ (u32)m.vertices.size() };
            assert(num_vertices);
            m.packed_vertices_static.reserve(num_vertices);

            for (u32 i{ 0 }; i < num_vertices; ++i) {
                vertex& v{ m.vertices[i] };
                const u8 signs{ (u8)((v.normal.z > 0.f) << 1) };
                const u16 normal_x{ (u16) math::pack_float<16>(v.normal.x, -1.f, 1.f) };
                const u16 normal_y{ (u16) math::pack_float<16>(v.normal.y, -1.f, 1.f) };

                m.packed_vertices_static.emplace_back(packed_vertex::vertex_static{
                    v.position,
                    {0, 0, 0},
                    signs,
                    normal_x,
                    normal_y,
                    {},
                    v.uv
                });
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
        for (auto& lod : scene.lod_groups) {
            for (auto& m : lod.meshes) {
                process_vertices(m, settings);
            }
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