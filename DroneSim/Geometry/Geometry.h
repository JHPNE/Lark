#pragma once
#include "../Common/CommonHeaders.h"

namespace drosim::tools {

  namespace packed_vertex {
    struct vertex_static {
      math::v3 position;
      u8 reserved[3];
      u8 t_sign;
      u16 normal[2];
      u16 tangent[2];
      math::v2 uv;
    };
  }

  struct vertex {
    math::v4 tangent{};
    math::v3 position{};
    math::v3 normal{};
    math::v2 uv{};
  };

  struct mesh {
    util::vector<math::v3> positions;
    util::vector<math::v3> normals;
    util::vector<math::v4> tangents;
    util::vector<util::vector<math::v2>> uv_sets;

    util::vector<u32> raw_indices;

    util::vector<vertex> vertices;
    util::vector<u32> indices;

    std::string name;
    util::vector<packed_vertex::vertex_static> packed_vertices_static;
    f32 lod_threshold{ -1.f};
    u32 lod_id{u32_invalid_id};
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

  void process_scene(scene& scene, const geometry_import_settings& settings);
  void pack_data(const scene& scene, scene_data& data);

}