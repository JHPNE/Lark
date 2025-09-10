#include "EngineUtilities.h"
#include "Core/GameLoop.h"
#include "Geometry.h"
#include "Geometry/MeshPrimitives.h"
#include "Physics.h"
#include "Script.h"
#include "Transform.h"
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

using namespace lark;

namespace engine
{
std::unique_ptr<GameLoop> g_game_loop;
util::vector<bool> active_entities;

// Internal implementation of transform conversion
transform::init_info to_engine_transform(const transform_component &transform)
{
    transform::init_info info{};
    memcpy(&info.position[0], &transform.position[0], sizeof(float) * 3);
    memcpy(&info.scale[0], &transform.scale[0], sizeof(float) * 3);

    glm::vec3 euler_angles(transform.rotation[0], transform.rotation[1], transform.rotation[2]);
    glm::quat quaternion = glm::quat(euler_angles);

    info.rotation[0] = quaternion.x;
    info.rotation[1] = quaternion.y;
    info.rotation[2] = quaternion.z;
    info.rotation[3] = quaternion.w;

    return info;
}

// Internal implementation of script conversion
script::init_info to_engine_script(const script_component &script)
{
    script::init_info info{};
    info.script_creator = script.script_creator; // Direct assignment since types match
    return info;
}

geometry::init_info to_engine_geometry(const geometry_component &geometry)
{
    geometry::init_info info{};

    if (geometry.scene == nullptr || geometry.scene->lod_groups.empty())
        return info;

    info.is_dynamic = false;
    info.scene = std::make_shared<lark::tools::scene>();
    util::vector<lark::tools::lod_group> lod_groups;

    for (auto comp_lod_group : geometry.scene->lod_groups)
    {
        lark::tools::lod_group lod_group;
        lod_group.name = comp_lod_group.name;

        util::vector<lark::tools::mesh> meshes;
        for (auto mesh : comp_lod_group.meshes)
        {
            lark::tools::mesh m;
            m.name = mesh.name;
            m.positions = mesh.positions;
            m.normals = mesh.normals;
            m.tangents = mesh.tangents;
            m.uv_sets = mesh.uv_sets;
            m.raw_indices = mesh.raw_indices;

            util::vector<lark::tools::vertex> vertices;
            for (auto vertex : mesh.vertices)
            {
                lark::tools::vertex v;
                v.tangent = vertex.tangent;
                v.position = vertex.position;
                v.normal = vertex.normal;
                v.uv = vertex.uv;

                vertices.emplace_back(v);
            }
            m.vertices = vertices;
            m.indices = mesh.indices;
            m.lod_threshold = mesh.lod_threshold;
            m.lod_id = mesh.lod_id;
            m.is_dynamic = false;

            util::vector<lark::tools::packed_vertex::vertex_static> packed_vertices_static;
            m.packed_vertices_static = packed_vertices_static;

            meshes.emplace_back(m);
        }
        lod_group.meshes = meshes;
        lod_groups.emplace_back(lod_group);
    }

    info.scene->lod_groups = lod_groups;

    if (!geometry.scene->name.empty())
    {
        info.scene->name = geometry.scene->name;
    }

    return info;
}

physics::init_info to_engine_physics(const physics_component &physics) {}

game_entity::entity entity_from_id(id::id_type id)
{
    return game_entity::entity{game_entity::entity_id(id)};
}

bool is_entity_valid(id::id_type id)
{
    if (!id::is_valid(id))
        return false;
    const id::id_type index{id::index(id)};
    if (index >= active_entities.size())
        return false;
    return active_entities[index];
}

void remove_entity(id::id_type id)
{
    if (!is_entity_valid(id))
        return;

    game_entity::entity_id entity_id{id};
    if (game_entity::is_alive(entity_id))
    {
        auto entity = game_entity::entity{entity_id};
        if (auto script_comp = entity.script(); script_comp.is_valid())
        {
            script::remove(script_comp);
        }
        game_entity::remove(entity_id);
    }

    const auto index = id::index(id);
    active_entities[index] = false;
}

// Convert from ContentTools types to Engine types
lark::tools::primitive_mesh_type ConvertPrimitiveType(content_tools::PrimitiveMeshType type)
{
    return static_cast<lark::tools::primitive_mesh_type>(type);
}

void cleanup_engine_systems()
{
    std::vector<id::id_type> to_remove;
    for (id::id_type i = 0; i < active_entities.size(); ++i)
    {
        if (active_entities[i])
        {
            to_remove.push_back(i);
        }
    }

    for (auto id : to_remove)
    {
        remove_entity(id);
    }

    active_entities.clear();
    script::shutdown();
}
} // namespace engine