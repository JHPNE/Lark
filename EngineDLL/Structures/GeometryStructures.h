#pragma once

namespace content_tools {
// Match the engine's primitive types but in our own namespace
enum class PrimitiveMeshType {
  plane,
  cube,
  uv_sphere,
  ico_sphere,
  cylinder,
  capsule,

  count
};

struct GeometryImportSettings {
  float smoothing_angle = 178.0f;
  uint8_t calculate_normals = 0;
  uint8_t calculate_tangents = 1;
  uint8_t reverse_handedness = 0;
  uint8_t import_embeded_textures = 1;
  uint8_t import_animations = 1;
};

struct SceneData {
  uint8_t* buffer = nullptr;
  uint32_t buffer_size = 0;
  lark::tools::geometry_import_settings import_settings;
};

struct PrimitiveInitInfo {
  PrimitiveMeshType type;
  uint32_t segments[3] = {1, 1, 1};
  lark::math::v3 size{1, 1, 1};
  uint32_t lod = 0;
};

struct vertex_static {
  lark::math::v3 position;     ///< Vertex position in 3D space
  u8 reserved[3];        ///< Reserved for alignment
  u8 t_sign;            ///< Tangent sign bit
  u16 normal[2];        ///< Compressed normal vector
  u16 tangent[2];       ///< Compressed tangent vector
  lark::math::v2 uv;          ///< Texture coordinates
};

struct vertex {
  lark::math::v4 tangent{};    ///< Tangent vector with handedness
  lark::math::v3 position{};   ///< Vertex position
  lark::math::v3 normal{};     ///< Normal vector
  lark::math::v2 uv{};         ///< Texture coordinates
};

struct mesh {
  lark::util::vector<lark::math::v3> positions;     ///< Vertex positions
  lark::util::vector<lark::math::v3> normals;       ///< Vertex normals
  lark::util::vector<lark::math::v4> tangents;      ///< Vertex tangents
  lark::util::vector<lark::util::vector<lark::math::v2>> uv_sets;  ///< Multiple UV sets

  lark::util::vector<u32> raw_indices;        ///< Raw triangle indices

  lark::util::vector<vertex> vertices;        ///< Processed vertices
  lark::util::vector<u32> indices;           ///< Processed indices

  std::string name;                    ///< Mesh name
  lark::util::vector<vertex_static> packed_vertices_static;  ///< Packed vertices
  f32 lod_threshold{ -1.f };          ///< LOD switch threshold
  u32 lod_id{u32_invalid_id};         ///< LOD identifier
};

struct lod_group {
  std::string name;              ///< Group name
  lark::util::vector<mesh> meshes;     ///< Meshes at different LOD levels
};

struct scene {
  std::string name;                    ///< Scene name
  lark::util::vector<lod_group> lod_groups;  ///< LOD groups in the scene
};

struct scene_data {
  u8* buffer;                    ///< Raw data buffer
  u32 buffer_size;               ///< Size of the data buffer
  geometry_import_settings settings;  ///< Import settings used
};

}