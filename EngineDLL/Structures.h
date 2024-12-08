#pragma once
#include <CommonHeaders.h>

// Geometry Structures

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
  drosim::tools::geometry_import_settings import_settings;
};

struct PrimitiveInitInfo {
  PrimitiveMeshType type;
  uint32_t segments[3] = {1, 1, 1};
  drosim::math::v3 size{1, 1, 1};
  uint32_t lod = 0;
};
}

namespace drosim::editor {
struct Mesh {
  s32 vertex_size = 0;
  s32 vertex_count = 0;
  s32 index_size = 0;
  s32 index_count = 0;
  std::vector<u8> vertices;
  std::vector<u8> indices;
};

struct MeshLOD {
  std::string name;
  f32 lod_threshold = 0.0f;
  std::vector<std::shared_ptr<Mesh>> meshes;
};

struct LODGroup {
  std::string name;
  std::vector<std::shared_ptr<MeshLOD>> lods;
};
}

// Component Structures

struct transform_component {
  float position[3];
  float rotation[3];
  float scale[3];
};

struct script_component {
  drosim::script::detail::script_creator script_creator;  // Use the actual type instead of void*
};

enum GeometryType {
  PrimitiveType,
  ObjImport,
};

struct geometry_component {
  const char *name;
  const char *file_name;
  GeometryType type;
};

struct game_entity_descriptor {
  transform_component transform;
  script_component script;
  geometry_component geometry;
};