#include "MeshPrimitives.h"
#include "Geometry.h"

namespace lark::tools {
namespace {

  using namespace math;
  using primitive_mesh_creator = void(*)(scene&, const primitive_init_info& info);

  void create_plane(scene& scene, const primitive_init_info& info);
  void create_cube(scene& scene, const primitive_init_info& info);
  void create_uv_sphere(scene& scene, const primitive_init_info& info);
  void create_ico_sphere(scene& scene, const primitive_init_info& info);
  void create_cylinder(scene& scene, const primitive_init_info& info);
  void create_capsule(scene& scene, const primitive_init_info& info);


  primitive_mesh_creator creators[] {
    create_plane,
    create_cube,
    create_uv_sphere,
    create_ico_sphere,
    create_cylinder,
    create_capsule
  };

  static_assert(glm::countof(creators) == primitive_mesh_type::count);

  struct axis {
    enum : u32 {
      x = 0,
      y = 1,
      z = 2
    };
  };
  mesh create_plane(const primitive_init_info& info, u32 horizontal_index = axis::x
    , u32 vertical_index = axis::z ,bool flip_winding = false
    , v3 offset = {-0.5f, 0.f, -0.5f}, v2 u_range = {0.f, 1.f}, v2 v_range = {0.f, 1.f}) {

    assert(horizontal_index < 3 && vertical_index < 3);
    assert(horizontal_index != vertical_index);

    const u32 horizontal_count{glm::clamp(info.segments[horizontal_index], 1u, 10u)};
    const u32 vertical_count{glm::clamp(info.segments[vertical_index], 1u, 10u)};
    const f32 horizontal_step{ 1.f/horizontal_count};
    const f32 vertical_step{ 1.f/vertical_count};
    const f32 u_step{(u_range.y - u_range.x)/horizontal_count};
    const f32 v_step{(v_range.y - v_range.x)/vertical_count};

    mesh m{};
    util::vector<v2> uvs;

    for (u32 j = 0; j <= vertical_count; ++j)
      for (u32 i = 0; i <= horizontal_count; ++i) {
        v3 position{offset};
        f32* const as_array{ &position.x };
        as_array[horizontal_index] += i * horizontal_step;
        as_array[vertical_index] += j * vertical_step;
        m.positions.emplace_back(position.x * info.size.x, position.y * info.size.y, position.z * info.size.z);

        v2 uv{u_range.x, 1.f - v_range.x};
        uv.x += i * u_step;
        uv.y -= j * v_step;
        uvs.emplace_back(uv);
      }

    assert(m.positions.size() == (((u64)horizontal_count + 1) * ((u64)vertical_count + 1)));

    const u32 row_length{horizontal_count + 1}; //number of vertices in a row
    for (u32 j = 0; j < vertical_count; ++j) {
      u32 k{0};
      for (u32 i{k}; i < horizontal_count; ++i) {

        const u32 index[4] {
          i + j * row_length,
          i + (j + 1) * row_length,
          (i + 1) + j * row_length,
          (i + 1) + (j + 1) * row_length
        };

        m.raw_indices.emplace_back(index[0]);
        m.raw_indices.emplace_back(index[flip_winding ? 2 : 1]);
        m.raw_indices.emplace_back(index[flip_winding ? 1 : 2]);

        m.raw_indices.emplace_back(index[2]);
        m.raw_indices.emplace_back(index[flip_winding ? 3 : 1]);
        m.raw_indices.emplace_back(index[flip_winding ? 1 : 3]);
      }
      ++k;
    }

    const u32 num_indices{3 * 2 * horizontal_count * vertical_count};
    assert(m.raw_indices.size() == num_indices);

    m.uv_sets.resize(1);

    for (u32 i{0}; i < num_indices; ++i) {
      m.uv_sets[0].emplace_back(uvs[m.raw_indices[i]]);
    }

    return m;
  }

  mesh create_uv_sphere(const primitive_init_info& info)
  {
    const u32 phi_count{ glm::clamp(info.segments[axis::x], 3u, 64u) };
    const u32 theta_count{ glm::clamp(info.segments[axis::y], 2u, 64u) };
    const f32 theta_step{ pi / theta_count };
    const f32 phi_step{ glm::two_pi<float>() / phi_count };
    const u32 num_indices{ 2 * 3 * phi_count + 2 * 3 * phi_count * (theta_count - 2) };
    const u32 num_vertices{ 2 + phi_count * (theta_count - 1) };

    mesh m{};
    m.name = "uv_sphere";
    m.positions.resize(num_vertices);

    // Add the top vertex
    u32 c{ 0 };
    m.positions[c++] = { 0.f, info.size.y, 0.f };

    for (u32 j{ 1 }; j <= (theta_count - 1); ++j)
    {
        const f32 theta{ j * theta_step };
        for (u32 i{ 0 }; i < phi_count; ++i)
        {
            const f32 phi{ i * phi_step };
            m.positions[c++] = {
                info.size.x * glm::sin(theta) * glm::cos(phi),
                info.size.y * glm::cos(theta),
                -info.size.z * glm::sin(theta) * glm::sin(phi)
            };
        }
    }

    // Add the bottom vertex
    m.positions[c++] = { 0.f, -info.size.y, 0.f };
    assert(c == num_vertices);

    c = 0;
    m.raw_indices.resize(num_indices);
    util::vector<v2> uvs(num_indices);
    const f32 inv_theta_count{ 1.f / theta_count };
    const f32 inv_phi_count{ 1.f / phi_count };

    // Indices for the top cap, connecting the north pole to the first ring
    for (u32 i{ 0 }; i < phi_count - 1; ++i)
    {
        uvs[c] = { (2 * i + 1) * 0.5f * inv_phi_count, 1.f };
        m.raw_indices[c++] = 0;
        uvs[c] = { i * inv_phi_count, 1.f - inv_theta_count };
        m.raw_indices[c++] = i + 1;
        uvs[c] = { (i + 1) * inv_phi_count, 1.f - inv_theta_count };
        m.raw_indices[c++] = i + 2;
    }

    uvs[c] = { 1.f - 0.5f * inv_phi_count, 1.f };
    m.raw_indices[c++] = 0;
    uvs[c] = { 1.f - inv_phi_count, 1.f - inv_theta_count };
    m.raw_indices[c++] = phi_count;
    uvs[c] = { 1.f , 1.f - inv_theta_count };
    m.raw_indices[c++] = 1;

    // Indices for the section between the top and bottom rings
    for (u32 j{ 0 }; j < (theta_count - 2); ++j)
    {
        for (u32 i{ 0 }; i < (phi_count - 1); ++i)
        {
            const u32 index[4]{
                1 + i + j * phi_count,
                1 + i + (j + 1) * phi_count,
                1 + (i + 1) + (j + 1) * phi_count,
                1 + (i + 1) + j * phi_count
            };

            uvs[c] = { i * inv_phi_count, 1.f - (j + 1) * inv_theta_count };
            m.raw_indices[c++] = index[0];
            uvs[c] = { i * inv_phi_count, 1.f - (j + 2) * inv_theta_count };
            m.raw_indices[c++] = index[1];
            uvs[c] = { (i + 1) * inv_phi_count, 1.f - (j + 2) * inv_theta_count };
            m.raw_indices[c++] = index[2];

            uvs[c] = { i * inv_phi_count, 1.f - (j + 1) * inv_theta_count };
            m.raw_indices[c++] = index[0];
            uvs[c] = { (i + 1) * inv_phi_count, 1.f - (j + 2) * inv_theta_count };
            m.raw_indices[c++] = index[2];
            uvs[c] = { (i + 1) * inv_phi_count, 1.f - (j + 1) * inv_theta_count };
            m.raw_indices[c++] = index[3];
        }

        const u32 index[4]{
            phi_count + j * phi_count,
            phi_count + (j + 1) * phi_count,
            1 + (j + 1) * phi_count,
            1 + j * phi_count
        };

        uvs[c] = { 1.f - inv_phi_count, 1.f - (j + 1) * inv_theta_count };
        m.raw_indices[c++] = index[0];
        uvs[c] = { 1.f - inv_phi_count, 1.f - (j + 2) * inv_theta_count };
        m.raw_indices[c++] = index[1];
        uvs[c] = { 1.f, 1.f - (j + 2) * inv_theta_count };
        m.raw_indices[c++] = index[2];

        uvs[c] = { 1.f - inv_phi_count, 1.f - (j + 1) * inv_theta_count };
        m.raw_indices[c++] = index[0];
        uvs[c] = { 1.f, 1.f - (j + 2) * inv_theta_count };
        m.raw_indices[c++] = index[2];
        uvs[c] = { 1.f, 1.f - (j + 1) * inv_theta_count };
        m.raw_indices[c++] = index[3];
    }

    // Indices for the bottom cap, connecting the south posle to the last ring
    const u32 south_pole_index{ (u32)m.positions.size() - 1 };
    for (u32 i{ 0 }; i < (phi_count - 1); ++i)
    {
        uvs[c] = { (2 * i + 1) * 0.5f * inv_phi_count, 0.f };
        m.raw_indices[c++] = south_pole_index;
        uvs[c] = { (i + 1) * inv_phi_count, inv_theta_count };
        m.raw_indices[c++] = south_pole_index - phi_count + i + 1;
        uvs[c] = { i * inv_phi_count, inv_theta_count };
        m.raw_indices[c++] = south_pole_index - phi_count + i;
    }

    uvs[c] = { 1.f - 0.5f * inv_phi_count, 0.f };
    m.raw_indices[c++] = south_pole_index;
    uvs[c] = { 1.f, inv_theta_count };
    m.raw_indices[c++] = south_pole_index - phi_count;
    uvs[c] = { 1.f - inv_phi_count, inv_theta_count };
    m.raw_indices[c++] = south_pole_index - 1;

    assert(c == num_indices);

    m.uv_sets.emplace_back(uvs);

    return m;
  }

/**
 * @brief Creates a segmented cube mesh with precise vertex and face generation
 * @param info Primitive initialization parameters
 * @precondition info.size components must be positive non-zero values
 * @precondition info.segments components must be at least 1
 * @return Fully constructed mesh with validated geometry
 */
mesh create_cube(const primitive_init_info& info) {
    // PRECONDITION: All size components must be > 0
    assert(info.size.x > 0.0f && info.size.y > 0.0f && info.size.z > 0.0f);

    mesh m{};
    m.name = "cube";

    // Compute half extents so that the cube is centered at the origin.
    const v3 half = info.size * 0.5f;

    // Define the 6 faces of the cube. For each face we specify:
    //  - The normal (for lighting and flat shading)
    //  - The four corner positions in a consistent winding order
    //  - The UV coordinates for each vertex (we use (0,0)-(1,1))
    struct Face {
        v3 normal;
        v3 positions[4];
        v2 uvs[4];
    };

    Face faces[6] = {
        // Front face (+Z)
        {
            v3(0.f, 0.f, 1.f),
            {
                v3(-half.x, -half.y, half.z),
                v3( half.x, -half.y, half.z),
                v3( half.x,  half.y, half.z),
                v3(-half.x,  half.y, half.z)
            },
            {
                v2(0.f, 0.f),
                v2(1.f, 0.f),
                v2(1.f, 1.f),
                v2(0.f, 1.f)
            }
        },
        // Back face (-Z)
        {
            v3(0.f, 0.f, -1.f),
            {
                v3( half.x, -half.y, -half.z),
                v3(-half.x, -half.y, -half.z),
                v3(-half.x,  half.y, -half.z),
                v3( half.x,  half.y, -half.z)
            },
            {
                v2(0.f, 0.f),
                v2(1.f, 0.f),
                v2(1.f, 1.f),
                v2(0.f, 1.f)
            }
        },
        // Right face (+X)
        {
            v3(1.f, 0.f, 0.f),
            {
                v3(half.x, -half.y,  half.z),
                v3(half.x, -half.y, -half.z),
                v3(half.x,  half.y, -half.z),
                v3(half.x,  half.y,  half.z)
            },
            {
                v2(0.f, 0.f),
                v2(1.f, 0.f),
                v2(1.f, 1.f),
                v2(0.f, 1.f)
            }
        },
        // Left face (-X)
        {
            v3(-1.f, 0.f, 0.f),
            {
                v3(-half.x, -half.y, -half.z),
                v3(-half.x, -half.y,  half.z),
                v3(-half.x,  half.y,  half.z),
                v3(-half.x,  half.y, -half.z)
            },
            {
                v2(0.f, 0.f),
                v2(1.f, 0.f),
                v2(1.f, 1.f),
                v2(0.f, 1.f)
            }
        },
        // Top face (+Y)
        {
            v3(0.f, 1.f, 0.f),
            {
                v3(-half.x, half.y,  half.z),
                v3( half.x, half.y,  half.z),
                v3( half.x, half.y, -half.z),
                v3(-half.x, half.y, -half.z)
            },
            {
                v2(0.f, 0.f),
                v2(1.f, 0.f),
                v2(1.f, 1.f),
                v2(0.f, 1.f)
            }
        },
        // Bottom face (-Y)
        {
            v3(0.f, -1.f, 0.f),
            {
                v3(-half.x, -half.y, -half.z),
                v3( half.x, -half.y, -half.z),
                v3( half.x, -half.y,  half.z),
                v3(-half.x, -half.y,  half.z)
            },
            {
                v2(0.f, 0.f),
                v2(1.f, 0.f),
                v2(1.f, 1.f),
                v2(0.f, 1.f)
            }
        }
    };

    // Reserve space: 24 vertices and 36 indices
    m.positions.reserve(24);
    m.normals.reserve(24);
    util::vector<v2> allUVs;
    allUVs.reserve(24);
    m.raw_indices.reserve(36);

    // For each face, add its vertices and corresponding indices.
    // Vertices for a face are stored consecutively.
    for (u32 face = 0; face < 6; ++face) {
        const u32 baseIndex = static_cast<u32>(m.positions.size());
        for (u32 i = 0; i < 4; ++i) {
            m.positions.push_back(faces[face].positions[i]);
            m.normals.push_back(faces[face].normal);
            allUVs.push_back(faces[face].uvs[i]);
        }

        // Two triangles per face (clockwise winding order)
        // Triangle 1: vertices 0, 1, 2
        m.raw_indices.push_back(baseIndex + 0);
        m.raw_indices.push_back(baseIndex + 1);
        m.raw_indices.push_back(baseIndex + 2);

        // Triangle 2: vertices 0, 2, 3
        m.raw_indices.push_back(baseIndex + 0);
        m.raw_indices.push_back(baseIndex + 2);
        m.raw_indices.push_back(baseIndex + 3);
    }

    // Save the UVs into the mesh’s UV set (assumes uv_sets[0] exists)
    m.uv_sets.resize(1);
    m.uv_sets[0] = std::move(allUVs);

    // Optional: Validate vertex/index counts
    assert(m.positions.size() == 24);
    assert(m.normals.size() == 24);
    assert(m.raw_indices.size() == 36);
    assert(m.uv_sets[0].size() == 24);

    return m;
}

  void create_plane(scene& scene, const primitive_init_info& info) {
    lod_group lod{};
    lod.name = "plane";
    lod.meshes.emplace_back(create_plane(info));
    scene.lod_groups.emplace_back(lod);
  }

  void create_cube(scene& scene, const primitive_init_info& info) {
    lod_group lod{};
    lod.name = "cube";
    lod.meshes.emplace_back(create_cube(info));
    scene.lod_groups.emplace_back(lod);
  }
  void create_uv_sphere(scene& scene, const primitive_init_info& info) {
    lod_group lod{};
    lod.name = "uv_sphere";
    lod.meshes.emplace_back(create_uv_sphere(info));
    scene.lod_groups.emplace_back(lod);
  }
  void create_ico_sphere(scene& scene, const primitive_init_info& info) {

  }
  void create_cylinder(scene& scene, const primitive_init_info& info) {

  }
  void create_capsule(scene& scene, const primitive_init_info& info) {

  }

}

void CreatePrimitiveMesh(scene_data* data, primitive_init_info* info) {
  assert(data && info);
  assert(info->mesh_type < primitive_mesh_type::count);
  scene scene{};
  creators[info->mesh_type](scene, *info);

  data->settings.calculate_normals = 1;
  process_scene(scene, data->settings);
  pack_data(scene, *data);

};
}