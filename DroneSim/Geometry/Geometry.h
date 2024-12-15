/**
 * @file Geometry.h
 * @brief Core geometry system for 3D mesh handling
 * 
 * This file defines the core geometry structures and functions for handling
 * 3D meshes in the simulation. It includes support for LOD (Level of Detail),
 * vertex packing, and geometry processing.
 */

#pragma once
#include "../Common/CommonHeaders.h"

namespace drosim::tools {

    namespace packed_vertex {
        /**
         * @struct vertex_static
         * @brief Packed vertex format for efficient storage and rendering
         * 
         * This structure represents a vertex in a compressed format,
         * optimizing memory usage while maintaining necessary information
         * for rendering.
         */
        struct vertex_static {
            math::v3 position;     ///< Vertex position in 3D space
            u8 reserved[3];        ///< Reserved for alignment
            u8 t_sign;            ///< Tangent sign bit
            u16 normal[2];        ///< Compressed normal vector
            u16 tangent[2];       ///< Compressed tangent vector
            math::v2 uv;          ///< Texture coordinates
        };
    }

    /**
     * @struct vertex
     * @brief Full vertex structure with complete geometric information
     * 
     * This structure represents a vertex with full precision data,
     * used during geometry processing before packing.
     */
    struct vertex {
        math::v4 tangent{};    ///< Tangent vector with handedness
        math::v3 position{};   ///< Vertex position
        math::v3 normal{};     ///< Normal vector
        math::v2 uv{};         ///< Texture coordinates
    };

    /**
     * @struct mesh
     * @brief Represents a single 3D mesh with geometry and LOD data
     * 
     * Contains all geometric data for a mesh, including vertices,
     * indices, and LOD information. Supports both raw and packed
     * vertex formats.
     */
    struct mesh {
        util::vector<math::v3> positions;     ///< Vertex positions
        util::vector<math::v3> normals;       ///< Vertex normals
        util::vector<math::v4> tangents;      ///< Vertex tangents
        util::vector<util::vector<math::v2>> uv_sets;  ///< Multiple UV sets

        util::vector<u32> raw_indices;        ///< Raw triangle indices

        util::vector<vertex> vertices;        ///< Processed vertices
        util::vector<u32> indices;           ///< Processed indices

        std::string name;                    ///< Mesh name
        util::vector<packed_vertex::vertex_static> packed_vertices_static;  ///< Packed vertices
        f32 lod_threshold{ -1.f };          ///< LOD switch threshold
        u32 lod_id{u32_invalid_id};         ///< LOD identifier

        bool is_dynamic{ false };           ///< Whether this mesh supports dynamic updates

        /**
         * @brief Toggle dynamic mode for this mesh
         * @param dynamic Whether to enable dynamic mode
         * 
         * When enabled, the mesh supports runtime vertex position updates
         */
        void set_dynamic(bool dynamic);
    };

    /**
     * @struct lod_group
     * @brief Group of meshes representing different LOD levels
     * 
     * Contains multiple versions of the same mesh at different
     * detail levels for LOD rendering.
     */
    struct lod_group {
        std::string name;              ///< Group name
        util::vector<mesh> meshes;     ///< Meshes at different LOD levels
    };

    /**
     * @struct scene
     * @brief Collection of LOD groups forming a complete 3D scene
     * 
     * Top-level container for all geometry in a scene, organized
     * into LOD groups.
     */
    struct scene {
        std::string name;                    ///< Scene name
        util::vector<lod_group> lod_groups;  ///< LOD groups in the scene
    };

    /**
     * @struct geometry_import_settings
     * @brief Settings for geometry import and processing
     * 
     * Configuration options that control how geometry is imported
     * and processed, including LOD generation and vertex packing.
     */
    struct geometry_import_settings {
        f32 smoothing_angle;            ///< Angle threshold for normal smoothing
        bool calculate_normals;         ///< Whether to calculate normals
        bool calculate_tangents;        ///< Whether to calculate tangents
        bool reverse_handedness;        ///< Whether to reverse coordinate system handedness
        bool import_embeded_textures;   ///< Whether to import embedded textures
        bool import_animations;         ///< Whether to import animations
    };

    /**
     * @struct scene_data
     * @brief Container for processed scene data
     * 
     * Holds the final processed geometry data ready for rendering
     * or serialization.
     */
    struct scene_data {
        u8* buffer;                    ///< Raw data buffer
        u32 buffer_size;               ///< Size of the data buffer
        geometry_import_settings settings;  ///< Import settings used
    };

    /**
     * @brief Processes a scene according to import settings
     * @param scene Scene to process
     * @param settings Import settings to use
     * 
     * Applies geometry processing operations specified in the settings
     * to all meshes in the scene.
     */
    void process_scene(scene& scene, const geometry_import_settings& settings);

    /**
     * @brief Packs scene data into an optimized format
     * @param scene Scene to pack
     * @param data Output container for packed data
     * 
     * Converts the scene's geometry into an optimized format for
     * storage or transmission.
     */
    void pack_data(const scene& scene, scene_data& data);

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
                                 const geometry_import_settings& settings);
}