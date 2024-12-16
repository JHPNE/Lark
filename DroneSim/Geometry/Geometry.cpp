#include "Geometry.h"
#include <execution>
#include <map>

namespace drosim::tools {
    namespace {

        // Add this new function at the top of the anonymous namespace
        void clear_processed_vertex_data(tools::mesh& m) {
            m.vertices.clear();
            m.vertices.shrink_to_fit();
            m.indices.clear();
            m.indices.shrink_to_fit();
            m.normals.clear();
            m.normals.shrink_to_fit();
            m.tangents.clear();
            m.tangents.shrink_to_fit();
            m.packed_vertices_static.clear();
            m.packed_vertices_static.shrink_to_fit();
        }

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
            if (m.uv_sets.empty() || m.uv_sets[0].empty()) {
                // If no UV data, create default UVs (0,0) for all vertices
                if (!m.vertices.empty()) {
                    for (auto& vertex : m.vertices) {
                        vertex.uv = math::v2(0.0f, 0.0f);
                    }
                }
                return;
            }

            const u32 num_vertices = (u32)m.vertices.size();
            const u32 num_indices = (u32)m.indices.size();
            if (!num_vertices || !num_indices) return;

            // Ensure UV set has enough data
            if (m.uv_sets[0].size() < num_indices) {
                printf("Warning: UV data size mismatch. Expected %u, got %zu\n", 
                       num_indices, m.uv_sets[0].size());
                // Resize and fill with default values
                m.uv_sets[0].resize(num_indices, math::v2(0.0f, 0.0f));
            }

            // Rest of the existing process_uvs implementation...
            util::vector<vertex> new_vertices;
            new_vertices.reserve(num_indices); // Reserve space for worst case
            util::vector<u32> new_indices(num_indices);

            // Map to track unique vertex-UV combinations
            struct vertex_key {
                math::v3 position;
                math::v3 normal;
                math::v2 uv;

                bool operator<(const vertex_key& other) const {
                    if (position.x != other.position.x) return position.x < other.position.x;
                    if (position.y != other.position.y) return position.y < other.position.y;
                    if (position.z != other.position.z) return position.z < other.position.z;
                    if (normal.x != other.normal.x) return normal.x < other.normal.x;
                    if (normal.y != other.normal.y) return normal.y < other.normal.y;
                    if (normal.z != other.normal.z) return normal.z < other.normal.z;
                    if (uv.x != other.uv.x) return uv.x < other.uv.x;
                    return uv.y < other.uv.y;
                }
            };

            std::map<vertex_key, u32> vertex_map;

            // Process each index
            for (u32 i = 0; i < num_indices; ++i) {
                const vertex& v = m.vertices[m.indices[i]];
                vertex_key key{v.position, v.normal, m.uv_sets[0][i]};

                auto it = vertex_map.find(key);
                if (it == vertex_map.end()) {
                    // New unique vertex-UV combination
                    vertex new_v = v;
                    new_v.uv = m.uv_sets[0][i];
                    new_indices[i] = (u32)new_vertices.size();
                    vertex_map[key] = new_indices[i];
                    new_vertices.push_back(new_v);
                } else {
                    // Reuse existing vertex
                    new_indices[i] = it->second;
                }
            }

            // Update mesh with new data
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

            // Write vertex size and vertex count
            constexpr u32 vertex_size{ sizeof(packed_vertex::vertex_static) };
            s = vertex_size;
            memcpy(&buffer[at], &s, su32);
            at += su32;

            const u32 num_vertices{ (u32)m.vertices.size() };
            s = num_vertices;
            memcpy(&buffer[at], &s, su32);
            at += su32;

            // Write index size and index count
            const size_t index_size{ (num_vertices < (1 << 16)) ? sizeof(u16) : sizeof(u32) };
            s = (u32)index_size;
            memcpy(&buffer[at], &s, su32);
            at += su32;

            const u32 num_indices{ (u32)m.indices.size() };
            s = num_indices;
            memcpy(&buffer[at], &s, su32);
            at += su32;

            // **Write LOD threshold** before vertex data
            f32 threshold = m.lod_threshold;
            memcpy(&buffer[at], &threshold, sizeof(f32));
            at += sizeof(f32);

            // Write vertex data
            const u32 vertex_buffer_size = vertex_size * num_vertices;
            memcpy(&buffer[at], m.packed_vertices_static.data(), vertex_buffer_size);
            at += vertex_buffer_size;

            // Write index data
            const u32 index_buffer_size = (u32)(index_size * num_indices);
            void* data{ (void*)m.indices.data() };
            util::vector<u16> indices;

            if (index_size == sizeof(u16)) {
                indices.resize(num_indices);
                for (u32 i{ 0 }; i < num_indices; ++i) {
                    indices[i] = (u16)m.indices[i];
                }
                data = (void*)indices.data();
            }

            memcpy(&buffer[at], data, index_buffer_size);
            at += index_buffer_size;
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

    /**
     * @brief Update the positions of a single mesh within a scene, then reprocess geometry.
     *
     * Unlike the older `update_vertices` method, this function directly updates mesh data,
     * regenerates normals/tangents (if requested via settings), and repacks vertices.
     *
     * @param scn          Reference to the scene containing the mesh.
     * @param lod_index    Which LOD group to update.
     * @param mesh_index   Which mesh within that LOD group to update.
     * @param new_positions A vector of new vertex positions; must match the old position count.
     * @param settings     Geometry import settings (controls normal/tangent recalculation, smoothing angle, etc.).
     *
     * @return true if successful, false otherwise (e.g., invalid indices, size mismatch).
     */
    bool update_scene_mesh_positions(scene& scn,
                                     size_t lod_index,
                                     size_t mesh_index,
                                     const std::vector<math::v3>& new_positions,
                                     const geometry_import_settings& settings)
    {
        // Validate LOD index and mesh index.
        if (lod_index >= scn.lod_groups.size()) {
            printf("Invalid LOD index.");
            return false;
        }
        lod_group& lod = scn.lod_groups[lod_index];
        if (mesh_index >= lod.meshes.size()) {
            printf("Invalid mesh index.");
            return false;
        }

        // Fetch the mesh.
        mesh& m = lod.meshes[mesh_index];

        // Make sure the new_positions size matches the current vertex count.
        // If the mesh was previously processed, the authoritative vertex count
        // is m.positions.size() (the "raw" positions).
        if (new_positions.size() != m.positions.size()) {
            printf("new_positions.size() does not match mesh.positions.size()");
            return false;
        }

        // Update positions directly.
        m.positions = new_positions;

        // If raw_indices is empty, but we *do* have final indices, copy them back so
        // process_scene() can safely recalc normals/tangents from raw_indices.
        if (m.raw_indices.empty() && !m.indices.empty()) {
            m.raw_indices = m.indices;
        }

        // Clear only *processed* data (vertices, indices, normals, tangents, packed vertices).
        // DO NOT clear raw_indices if we want to recalc geometry properly.
        m.vertices.clear();
        m.vertices.shrink_to_fit();

        m.indices.clear();
        m.indices.shrink_to_fit();

        m.normals.clear();
        m.normals.shrink_to_fit();

        m.tangents.clear();
        m.tangents.shrink_to_fit();

        m.packed_vertices_static.clear();
        m.packed_vertices_static.shrink_to_fit();

        // Usually we preserve UV sets (m.uv_sets) unless you specifically want to modify them.

        // Initialize UV sets properly
        if (m.uv_sets.empty()) {
            m.uv_sets.resize(1);
            // Initialize UVs to match the number of raw indices, not processed indices
            m.uv_sets[0].resize(m.raw_indices.size(), math::v2(0.0f, 0.0f));
        } else if (m.uv_sets[0].size() != m.raw_indices.size()) {
            // Ensure UV set matches the number of indices
            m.uv_sets[0].resize(m.raw_indices.size(), math::v2(0.0f, 0.0f));
        }
        // Now reprocess the entire scene to recalc normals/tangents (if requested),
        // rebuild vertex/index buffers, and pack vertex data.
        process_scene(scn, settings);

        return true;
    }
}