#include "Entity.h"
#include "Geometry.h"
#include "Physics.h"
#include "Script.h"
#include "Transform.h"

namespace lark::game_entity
{
// private alternative to static
namespace
{
util::vector<transform::component> transforms;
util::vector<script::component> scripts;
util::vector<geometry::component> geometries;
util::vector<physics::component> physics_container;

std::vector<id::generation_type> generations;
util::deque<entity_id> free_ids;

util::vector<entity_id> active_entities;
} // namespace

entity create(entity_info info)
{
    assert(info.transform); // transform is required

    entity_id id;
    if (free_ids.size() > id::min_deleted_elements)
    {
        id = free_ids.front();
        assert(!is_alive(id));
        free_ids.pop_front();
        id = entity_id{id::new_generation(id)};
        ++generations[id::index(id)];
    }
    else
    {
        id = entity_id{(id::id_type)generations.size()};
        generations.push_back(0);

        // Resize Components
        // emplace isntead of resize for memory allocations
        transforms.emplace_back();
        scripts.emplace_back();
        geometries.emplace_back();
        physics_container.emplace_back();
    }

    const entity new_entity{id};
    const id::id_type index{id::index(id)};

    // Create Transform Component
    assert(!transforms[index].is_valid());
    transforms[index] = transform::create(*info.transform, new_entity);
    if (!transforms[index].is_valid())
        return {};

    // Create Script Component
    if (info.script && info.script->script_creator)
    {
        assert(!scripts[index].is_valid());
        scripts[index] = script::create(*info.script, new_entity);
        assert(scripts[index].is_valid());
    }

    // Create Geometry Component
    if (info.geometry && info.geometry->scene)
    {
        assert(!geometries[index].is_valid());
        geometries[index] = geometry::create(*info.geometry, new_entity);
    }

    // check if geometry is existing then drone component is available
    if (info.physics && info.physics->scene)
    {
        assert(!physics_container[index].is_valid());
        physics_container[index] = physics::create(*info.physics, new_entity);
    }

    if (new_entity.is_valid())
    {
        active_entities.push_back(new_entity.get_id());
    }

    return new_entity;
}

void remove(entity_id id)
{
    const id::id_type index{id::index(id)};
    assert(is_alive(id));

    // First invalidate any script component
    if (scripts[index].is_valid())
    {
        auto script_copy = scripts[index]; // Make a copy before invalidating
        scripts[index] = {};               // Invalidate first
        script::remove(script_copy);       // Then remove using the copy
    }

    if (geometries[index].is_valid())
    {
        auto geometry_copy = geometries[index];
        geometries[index] = {};
        geometry::remove(geometry_copy);
    }

    if (physics_container[index].is_valid())
    {
        auto physics_copy = physics_container[index];
        physics_container[index] = {};
        physics::remove(physics_copy);
    }

    transform::remove(transforms[index]);
    transforms[index] = {};

    if (generations[index] < id::max_generation)
    {
        free_ids.push_back(id);
    }

    auto it = std::find(active_entities.begin(), active_entities.end(), id);
    if (it != active_entities.end())
    {
        util::erase_unordered(active_entities, it - active_entities.begin());
    }
}

bool updateEntity(entity_id id, entity_info info)
{
    const id::id_type index{id::index(id)};
    assert(is_alive(id));

    const entity updated_entity{id};

    auto transform_comp = updated_entity.transform();
    if (!transform_comp.is_valid())
        return false;

    transform_comp.set_position(math::v3(info.transform->position[0], info.transform->position[1],
                                         info.transform->position[2]));

    math::v3 euler(info.transform->rotation[0], info.transform->rotation[1],
                   info.transform->rotation[2]);
    glm::quat quat = glm::quat(glm::radians(euler));
    transform_comp.set_rotation(math::v4(quat.x, quat.y, quat.z, quat.w));

    transform_comp.set_scale(
        math::v3(info.transform->scale[0], info.transform->scale[1], info.transform->scale[2]));

    // check if there is any script content
    if (info.script && info.script->script_creator)
    {
        // check if there is already and existing script for that id then delete it
        if (scripts[index].is_valid())
        {
            // delete part
            auto script_copy = scripts[index];
            scripts[index] = {};
            script::remove(script_copy);
        }

        // creating new part
        assert(!scripts[index].is_valid());
        scripts[index] = script::create(*info.script, updated_entity);
        assert(scripts[index].is_valid());
    }

    if (info.geometry && info.geometry->scene)
    {

        if (geometries[index].is_valid())
        {
            auto geometry_copy = geometries[index];
            geometries[index] = {};
            geometry::remove(geometry_copy);
        }

        assert(!geometries[index].is_valid());
        geometries[index] = geometry::create(*info.geometry, updated_entity);
        assert(geometries[index].is_valid());
    }

    return true;
}

const util::vector<entity_id> &get_active_entities() { return active_entities; }

bool is_alive(entity_id id)
{
    assert(id::is_valid(id));
    const id::id_type index{id::index(id)};
    assert(index < generations.size());
    return (generations[index] == id::generation(id) && transforms[index].is_valid());
}

transform::component entity::transform() const
{
    assert(is_alive(_id));
    const id::id_type index{id::index(_id)};
    return transforms[index];
}

script::component entity::script() const
{
    assert(is_alive(_id));
    return scripts[id::index(_id)];
}

geometry::component entity::geometry() const
{
    assert(is_alive(_id));
    return geometries[id::index(_id)];
}

physics::component entity::physics() const
{
    assert(is_alive(_id));
    return physics_container[id::index(_id)];
}
} // namespace lark::game_entity
