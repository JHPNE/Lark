#pragma once
#include "../Common/CommonHeaders.h"

namespace drosim::tools {

struct mesh {
  util::vector<math::v3> positions;
  util::vector<math::v3> normals;
  util::vector<math::v4> tangents;
  util::vector<util::vector<math::v2>> uv_sets;

  util::vector<u32> raw_indices;
};

struct lod_group {
  std::string name;
  util::vector<mesh> meshes;
};

struct scene {
  std::string name;
  util::vector<lod_group> lod_groups;
};

struct geometry_import_settings {
  f32 smoothing_angle;
  u8 calculate_normals;
  u8 calculate_tangents;
  u8 reverse_handedness;
  u8 import_embeded_textures;
  u8 import_animations;
};

struct scene_data {
  u8* buffer;
  u32 buffer_size;
  geometry_import_settings settings;
};



}