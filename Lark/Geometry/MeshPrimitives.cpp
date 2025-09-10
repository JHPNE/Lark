#include "MeshPrimitives.h"
#include "Geometry.h"

namespace lark::tools
{
namespace
{

using namespace math;
using primitive_mesh_creator = void (*)(scene &, const primitive_init_info &info);

void create_plane(scene &scene, const primitive_init_info &info);
void create_cube(scene &scene, const primitive_init_info &info);
void create_uv_sphere(scene &scene, const primitive_init_info &info);
void create_ico_sphere(scene &scene, const primitive_init_info &info);
void create_cylinder(scene &scene, const primitive_init_info &info);
void create_capsule(scene &scene, const primitive_init_info &info);

primitive_mesh_creator creators[]{create_plane,      create_cube,     create_uv_sphere,
                                  create_ico_sphere, create_cylinder, create_capsule};

static_assert(glm::countof(creators) == primitive_mesh_type::count);

struct axis
{
    enum : u32
    {
        x = 0,
        y = 1,
        z = 2
    };
};
mesh create_plane(const primitive_init_info &info, u32 horizontal_index = axis::x,
                  u32 vertical_index = axis::z, bool flip_winding = false,
                  v3 offset = {-0.5f, 0.f, -0.5f}, v2 u_range = {0.f, 1.f}, v2 v_range = {0.f, 1.f})
{

    assert(horizontal_index < 3 && vertical_index < 3);
    assert(horizontal_index != vertical_index);

    const u32 horizontal_count{glm::clamp(info.segments[horizontal_index], 1u, 10u)};
    const u32 vertical_count{glm::clamp(info.segments[vertical_index], 1u, 10u)};
    const f32 horizontal_step{1.f / horizontal_count};
    const f32 vertical_step{1.f / vertical_count};
    const f32 u_step{(u_range.y - u_range.x) / horizontal_count};
    const f32 v_step{(v_range.y - v_range.x) / vertical_count};

    mesh m{};
    util::vector<v2> uvs;

    for (u32 j = 0; j <= vertical_count; ++j)
        for (u32 i = 0; i <= horizontal_count; ++i)
        {
            v3 position{offset};
            f32 *const as_array{&position.x};
            as_array[horizontal_index] += i * horizontal_step;
            as_array[vertical_index] += j * vertical_step;
            m.positions.emplace_back(position.x * info.size.x, position.y * info.size.y,
                                     position.z * info.size.z);

            v2 uv{u_range.x, 1.f - v_range.x};
            uv.x += i * u_step;
            uv.y -= j * v_step;
            uvs.emplace_back(uv);
        }

    assert(m.positions.size() == (((u64)horizontal_count + 1) * ((u64)vertical_count + 1)));

    const u32 row_length{horizontal_count + 1}; // number of vertices in a row
    for (u32 j = 0; j < vertical_count; ++j)
    {
        u32 k{0};
        for (u32 i{k}; i < horizontal_count; ++i)
        {

            const u32 index[4]{i + j * row_length, i + (j + 1) * row_length,
                               (i + 1) + j * row_length, (i + 1) + (j + 1) * row_length};

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

    for (u32 i{0}; i < num_indices; ++i)
    {
        m.uv_sets[0].emplace_back(uvs[m.raw_indices[i]]);
    }

    return m;
}

mesh create_uv_sphere(const primitive_init_info &info)
{
    const u32 phi_count{glm::clamp(info.segments[axis::x], 3u, 64u)};
    const u32 theta_count{glm::clamp(info.segments[axis::y], 2u, 64u)};
    const f32 theta_step{pi / theta_count};
    const f32 phi_step{glm::two_pi<float>() / phi_count};
    const u32 num_indices{2 * 3 * phi_count + 2 * 3 * phi_count * (theta_count - 2)};
    const u32 num_vertices{2 + phi_count * (theta_count - 1)};

    mesh m{};
    m.name = "uv_sphere";
    m.positions.resize(num_vertices);

    // Add the top vertex
    u32 c{0};
    m.positions[c++] = {0.f, info.size.y, 0.f};

    for (u32 j{1}; j <= (theta_count - 1); ++j)
    {
        const f32 theta{j * theta_step};
        for (u32 i{0}; i < phi_count; ++i)
        {
            const f32 phi{i * phi_step};
            m.positions[c++] = {info.size.x * glm::sin(theta) * glm::cos(phi),
                                info.size.y * glm::cos(theta),
                                -info.size.z * glm::sin(theta) * glm::sin(phi)};
        }
    }

    // Add the bottom vertex
    m.positions[c++] = {0.f, -info.size.y, 0.f};
    assert(c == num_vertices);

    c = 0;
    m.raw_indices.resize(num_indices);
    util::vector<v2> uvs(num_indices);
    const f32 inv_theta_count{1.f / theta_count};
    const f32 inv_phi_count{1.f / phi_count};

    // Indices for the top cap, connecting the north pole to the first ring
    for (u32 i{0}; i < phi_count - 1; ++i)
    {
        uvs[c] = {(2 * i + 1) * 0.5f * inv_phi_count, 1.f};
        m.raw_indices[c++] = 0;
        uvs[c] = {i * inv_phi_count, 1.f - inv_theta_count};
        m.raw_indices[c++] = i + 1;
        uvs[c] = {(i + 1) * inv_phi_count, 1.f - inv_theta_count};
        m.raw_indices[c++] = i + 2;
    }

    uvs[c] = {1.f - 0.5f * inv_phi_count, 1.f};
    m.raw_indices[c++] = 0;
    uvs[c] = {1.f - inv_phi_count, 1.f - inv_theta_count};
    m.raw_indices[c++] = phi_count;
    uvs[c] = {1.f, 1.f - inv_theta_count};
    m.raw_indices[c++] = 1;

    // Indices for the section between the top and bottom rings
    for (u32 j{0}; j < (theta_count - 2); ++j)
    {
        for (u32 i{0}; i < (phi_count - 1); ++i)
        {
            const u32 index[4]{1 + i + j * phi_count, 1 + i + (j + 1) * phi_count,
                               1 + (i + 1) + (j + 1) * phi_count, 1 + (i + 1) + j * phi_count};

            uvs[c] = {i * inv_phi_count, 1.f - (j + 1) * inv_theta_count};
            m.raw_indices[c++] = index[0];
            uvs[c] = {i * inv_phi_count, 1.f - (j + 2) * inv_theta_count};
            m.raw_indices[c++] = index[1];
            uvs[c] = {(i + 1) * inv_phi_count, 1.f - (j + 2) * inv_theta_count};
            m.raw_indices[c++] = index[2];

            uvs[c] = {i * inv_phi_count, 1.f - (j + 1) * inv_theta_count};
            m.raw_indices[c++] = index[0];
            uvs[c] = {(i + 1) * inv_phi_count, 1.f - (j + 2) * inv_theta_count};
            m.raw_indices[c++] = index[2];
            uvs[c] = {(i + 1) * inv_phi_count, 1.f - (j + 1) * inv_theta_count};
            m.raw_indices[c++] = index[3];
        }

        const u32 index[4]{phi_count + j * phi_count, phi_count + (j + 1) * phi_count,
                           1 + (j + 1) * phi_count, 1 + j * phi_count};

        uvs[c] = {1.f - inv_phi_count, 1.f - (j + 1) * inv_theta_count};
        m.raw_indices[c++] = index[0];
        uvs[c] = {1.f - inv_phi_count, 1.f - (j + 2) * inv_theta_count};
        m.raw_indices[c++] = index[1];
        uvs[c] = {1.f, 1.f - (j + 2) * inv_theta_count};
        m.raw_indices[c++] = index[2];

        uvs[c] = {1.f - inv_phi_count, 1.f - (j + 1) * inv_theta_count};
        m.raw_indices[c++] = index[0];
        uvs[c] = {1.f, 1.f - (j + 2) * inv_theta_count};
        m.raw_indices[c++] = index[2];
        uvs[c] = {1.f, 1.f - (j + 1) * inv_theta_count};
        m.raw_indices[c++] = index[3];
    }

    // Indices for the bottom cap, connecting the south posle to the last ring
    const u32 south_pole_index{(u32)m.positions.size() - 1};
    for (u32 i{0}; i < (phi_count - 1); ++i)
    {
        uvs[c] = {(2 * i + 1) * 0.5f * inv_phi_count, 0.f};
        m.raw_indices[c++] = south_pole_index;
        uvs[c] = {(i + 1) * inv_phi_count, inv_theta_count};
        m.raw_indices[c++] = south_pole_index - phi_count + i + 1;
        uvs[c] = {i * inv_phi_count, inv_theta_count};
        m.raw_indices[c++] = south_pole_index - phi_count + i;
    }

    uvs[c] = {1.f - 0.5f * inv_phi_count, 0.f};
    m.raw_indices[c++] = south_pole_index;
    uvs[c] = {1.f, inv_theta_count};
    m.raw_indices[c++] = south_pole_index - phi_count;
    uvs[c] = {1.f - inv_phi_count, inv_theta_count};
    m.raw_indices[c++] = south_pole_index - 1;

    assert(c == num_indices);

    m.uv_sets.emplace_back(uvs);

    return m;
}

//------------------------------------------------------------------------
// Helper: Add a subdivided quad (face) into a mesh.
// The face is defined by a center point, two unit–direction vectors (right and up)
// and half–extents along each. The grid is defined by segRight and segUp divisions.
//
// UV coordinates are generated in [0,1]×[0,1] across the face.
// All vertices on this face get the same normal.
//------------------------------------------------------------------------
void add_face(mesh &m, const v3 &center, const v3 &normal,
              const v3 &right,  // local “horizontal” direction (unit length)
              const v3 &up,     // local “vertical” direction (unit length)
              float halfWidth,  // half–extent along the right direction
              float halfHeight, // half–extent along the up direction
              u32 segRight, u32 segUp)
{
    // Remember the starting index for this face:
    u32 baseIndex = static_cast<u32>(m.positions.size());
    // Create grid vertices:
    for (u32 j = 0; j <= segUp; ++j)
    {
        // t goes from 0 to 1 vertically
        float t = float(j) / float(segUp);
        // Interpolate vertical offset from -halfHeight to +halfHeight
        float offsetV = glm::mix(-halfHeight, halfHeight, t);
        for (u32 i = 0; i <= segRight; ++i)
        {
            float s = float(i) / float(segRight);
            float offsetU = glm::mix(-halfWidth, halfWidth, s);
            v3 pos = center + right * offsetU + up * offsetV;
            m.positions.push_back(pos);
            m.normals.push_back(normal);
            // UV coordinates – flip vertical axis if desired.
            m.uv_sets[0].push_back({s, t});
        }
    }
    // Create indices (two triangles per quad):
    for (u32 j = 0; j < segUp; ++j)
    {
        for (u32 i = 0; i < segRight; ++i)
        {
            u32 i0 = baseIndex + j * (segRight + 1) + i;
            u32 i1 = i0 + 1;
            u32 i2 = i0 + (segRight + 1);
            u32 i3 = i2 + 1;
            // Triangle 1
            m.raw_indices.push_back(i0);
            m.raw_indices.push_back(i1);
            m.raw_indices.push_back(i2);
            // Triangle 2
            m.raw_indices.push_back(i1);
            m.raw_indices.push_back(i3);
            m.raw_indices.push_back(i2);
        }
    }
}

//------------------------------------------------------------------------
// Modified segmented cube generator.
//
// We assume that info.size gives the overall dimensions,
// and info.segments is a vector of three values:
//   - segments[0]: subdivisions along X,
//   - segments[1]: subdivisions along Y,
//   - segments[2]: subdivisions along Z.
//
// We build each face as a subdivided plane with the following scheme:
//   Front and Back faces: horizontal: segments[0] (X), vertical: segments[1] (Y)
//   Right and Left faces: horizontal: segments[2] (Z), vertical: segments[1] (Y)
//   Top and Bottom faces: horizontal: segments[0] (X), vertical: segments[2] (Z)
//------------------------------------------------------------------------
mesh create_cube(const primitive_init_info &info)
{
    // PRECONDITION: All size components must be > 0
    assert(info.size.x > 0.0f && info.size.y > 0.0f && info.size.z > 0.0f);

    mesh m{};
    m.name = "cube";
    // Reserve one UV set.
    m.uv_sets.resize(1);

    // Compute half extents so that the cube is centered at the origin.
    const v3 half = info.size * 0.5f;

    // Determine segments per axis (ensure at least 1 subdivision per axis)
    u32 segX = glm::max(info.segments[0], 1u);
    u32 segY = glm::max(info.segments[1], 1u);
    u32 segZ = glm::max(info.segments[2], 1u);

    // Front face (+Z): center at (0,0,half.z)
    {
        v3 center = {0.f, 0.f, half.z};
        v3 normal = {0.f, 0.f, 1.f};
        v3 right = {1.f, 0.f, 0.f}; // from -half.x to +half.x
        v3 up = {0.f, 1.f, 0.f};    // from -half.y to +half.y
        add_face(m, center, normal, right, up, half.x, half.y, segX, segY);
    }
    // Back face (-Z): center at (0,0,-half.z)
    {
        v3 center = {0.f, 0.f, -half.z};
        // Note: for proper outward normals, the face normal is reversed.
        v3 normal = {0.f, 0.f, -1.f};
        // To keep winding consistent, we flip the right vector.
        v3 right = {-1.f, 0.f, 0.f}; // from +half.x to -half.x
        v3 up = {0.f, 1.f, 0.f};
        add_face(m, center, normal, right, up, half.x, half.y, segX, segY);
    }
    // Right face (+X): center at (half.x,0,0)
    {
        v3 center = {half.x, 0.f, 0.f};
        v3 normal = {1.f, 0.f, 0.f};
        // For the right face, the “horizontal” direction is along -Z
        v3 right = {0.f, 0.f, -1.f}; // from -half.z to +half.z
        v3 up = {0.f, 1.f, 0.f};
        add_face(m, center, normal, right, up, half.z, half.y, segZ, segY);
    }
    // Left face (-X): center at (-half.x,0,0)
    {
        v3 center = {-half.x, 0.f, 0.f};
        v3 normal = {-1.f, 0.f, 0.f};
        // For left face, horizontal goes from +half.z to -half.z
        v3 right = {0.f, 0.f, 1.f};
        v3 up = {0.f, 1.f, 0.f};
        add_face(m, center, normal, right, up, half.z, half.y, segZ, segY);
    }
    // Top face (+Y): center at (0,half.y,0)
    {
        v3 center = {0.f, half.y, 0.f};
        v3 normal = {0.f, 1.f, 0.f};
        // For the top face, horizontal is along X and vertical is along -Z.
        v3 right = {1.f, 0.f, 0.f}; // from -half.x to +half.x
        v3 up = {0.f, 0.f, -1.f};   // from +half.z to -half.z
        add_face(m, center, normal, right, up, half.x, half.z, segX, segZ);
    }
    // Bottom face (-Y): center at (0,-half.y,0)
    {
        v3 center = {0.f, -half.y, 0.f};
        v3 normal = {0.f, -1.f, 0.f};
        // For bottom face, horizontal is along X and vertical is along +Z.
        v3 right = {1.f, 0.f, 0.f}; // from -half.x to +half.x
        v3 up = {0.f, 0.f, 1.f};    // from -half.z to +half.z
        add_face(m, center, normal, right, up, half.x, half.z, segX, segZ);
    }

    // (Optional) You can add assertions to validate expected vertex/index counts.
    return m;
}

//------------------------------------------------------------------------
// New cylinder generator.
//
// We assume the cylinder is Y–axis–aligned, centered at the origin.
// info.size.y is the full height, and info.size.x (or z) gives the diameter
// of the base (so the radius is half of that).
//
// We use:
//    - radialSegments = info.segments[0] (minimum 3)
//    - heightSegments = info.segments[1] (minimum 1)
// Caps are generated as separate triangle fans.
//------------------------------------------------------------------------
mesh create_cylinder(const primitive_init_info &info)
{
    mesh m{};
    m.name = "cylinder";
    m.uv_sets.resize(1);

    // Parameters
    const u32 phi_count = glm::clamp(info.segments[0], 3u, 64u);
    const u32 height_segments = glm::max(info.segments[1], 1u);
    const float radius = info.size.x * 0.5f;
    const float halfHeight = info.size.y * 0.5f;

    const u32 num_vertices = 2 + phi_count * (height_segments + 1);
    const u32 num_indices = 2 * 3 * phi_count + 2 * 3 * phi_count * height_segments;

    m.positions.reserve(num_vertices);
    m.normals.reserve(num_vertices);
    m.uv_sets[0].reserve(num_vertices);
    m.raw_indices.reserve(num_indices);

    // Add top center vertex
    u32 current_vertex = 0;
    m.positions.push_back({0.f, halfHeight, 0.f});
    m.normals.push_back({0.f, 1.f, 0.f});
    m.uv_sets[0].push_back({0.5f, 0.5f});
    current_vertex++;

    // Generate vertices for height rings
    for (u32 j = 0; j <= height_segments; ++j)
    {
        const float v = static_cast<float>(j) / height_segments;
        const float y = glm::mix(halfHeight, -halfHeight, v);

        for (u32 i = 0; i < phi_count; ++i)
        {
            const float u = static_cast<float>(i) / phi_count;
            const float phi = u * glm::two_pi<float>();
            const float cos_phi = glm::cos(phi);
            const float sin_phi = glm::sin(phi);

            // Position
            m.positions.push_back({radius * cos_phi, y, radius * sin_phi});

            // Normal - vertical for caps, outward for sides
            if (j == 0)
            {
                m.normals.push_back({0.f, 1.f, 0.f});
            }
            else if (j == height_segments)
            {
                m.normals.push_back({0.f, -1.f, 0.f});
            }
            else
            {
                m.normals.push_back(glm::normalize(v3{cos_phi, 0.f, sin_phi}));
            }

            // UV coordinates
            if (j == 0 || j == height_segments)
            {
                // Cap UVs - circular mapping
                m.uv_sets[0].push_back({cos_phi * 0.5f + 0.5f, sin_phi * 0.5f + 0.5f});
            }
            else
            {
                // Side UVs - cylindrical mapping
                m.uv_sets[0].push_back({u, v});
            }
            current_vertex++;
        }
    }

    // Add bottom center vertex
    m.positions.push_back({0.f, -halfHeight, 0.f});
    m.normals.push_back({0.f, -1.f, 0.f});
    m.uv_sets[0].push_back({0.5f, 0.5f});

    // Generate indices
    u32 index_count = 0;

    // Top cap - CCW when viewed from above
    const u32 top_center = 0;
    const u32 top_ring_start = 1;
    for (u32 i = 0; i < phi_count; ++i)
    {
        const u32 next_i = (i + 1) % phi_count;
        m.raw_indices.push_back(top_center);
        m.raw_indices.push_back(top_ring_start + next_i);
        m.raw_indices.push_back(top_ring_start + i);
        index_count += 3;
    }

    // Side faces - ensure CCW when viewed from outside
    for (u32 j = 0; j < height_segments; ++j)
    {
        const u32 ring_start = 1 + j * phi_count;
        const u32 next_ring_start = ring_start + phi_count;

        for (u32 i = 0; i < phi_count; ++i)
        {
            const u32 next_i = (i + 1) % phi_count;
            const u32 curr_ring_curr = ring_start + i;
            const u32 curr_ring_next = ring_start + next_i;
            const u32 next_ring_curr = next_ring_start + i;
            const u32 next_ring_next = next_ring_start + next_i;

            // First triangle
            m.raw_indices.push_back(curr_ring_curr);
            m.raw_indices.push_back(curr_ring_next);
            m.raw_indices.push_back(next_ring_curr);

            // Second triangle
            m.raw_indices.push_back(curr_ring_next);
            m.raw_indices.push_back(next_ring_next);
            m.raw_indices.push_back(next_ring_curr);

            index_count += 6;
        }
    }

    // Bottom cap - CCW when viewed from below
    const u32 bottom_center = current_vertex;
    const u32 bottom_ring_start = bottom_center - phi_count;
    for (u32 i = 0; i < phi_count; ++i)
    {
        const u32 next_i = (i + 1) % phi_count;
        m.raw_indices.push_back(bottom_center);
        m.raw_indices.push_back(bottom_ring_start + i);
        m.raw_indices.push_back(bottom_ring_start + next_i);
        index_count += 3;
    }

    assert(index_count == num_indices);
    assert(current_vertex + 1 == num_vertices);

    return m;
}

void create_plane(scene &scene, const primitive_init_info &info)
{
    lod_group lod{};
    lod.name = "plane";
    lod.meshes.emplace_back(create_plane(info));
    scene.lod_groups.emplace_back(lod);
}

void create_cube(scene &scene, const primitive_init_info &info)
{
    lod_group lod{};
    lod.name = "cube";
    lod.meshes.emplace_back(create_cube(info));
    scene.lod_groups.emplace_back(lod);
}
void create_uv_sphere(scene &scene, const primitive_init_info &info)
{
    lod_group lod{};
    lod.name = "uv_sphere";
    lod.meshes.emplace_back(create_uv_sphere(info));
    scene.lod_groups.emplace_back(lod);
}
void create_ico_sphere(scene &scene, const primitive_init_info &info) {}
void create_cylinder(scene &scene, const primitive_init_info &info)
{
    lod_group lod{};
    lod.name = "cylinder";
    lod.meshes.emplace_back(create_cylinder(info));
    scene.lod_groups.emplace_back(lod);
}

void create_capsule(scene &scene, const primitive_init_info &info) {}

} // namespace

void CreatePrimitiveMesh(scene_data *data, primitive_init_info *info)
{
    assert(data && info);
    assert(info->mesh_type < primitive_mesh_type::count);
    scene scene{};
    creators[info->mesh_type](scene, *info);

    data->settings.calculate_normals = 1;
    process_scene(scene, data->settings);
    pack_data(scene, *data);
};
} // namespace lark::tools