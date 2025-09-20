#include "Physics.h"
#include <utility>
#include "PhysicExtension/Event/PhysicEvent.h"

namespace lark::physics
{
namespace
{
    struct physics_data
    {
        bool is_valid{false};
        btRigidBody *body{nullptr};
        float mass{1.0f};
    };

    util::vector<physics_data> physics_components;
    util::vector<id::id_type> id_mapping;
    util::vector<id::generation_type> generations;
    std::deque<physics_id> free_ids;

    bool exists(physics_id id)
    {
        assert(id::is_valid(id));
        const id::id_type index{id::index(id)};
        assert(index < generations.size());
        return (id::is_valid(id_mapping[index]) && generations[index] == id::generation(id) &&
                physics_components[id_mapping[index]].is_valid);
    }

    btConvexHullShape* extract_shape(const lod_group& group)
    {
        if (group.meshes.empty())
            return nullptr;

        const auto& mesh = group.meshes[0];
        auto* shape = new btConvexHullShape();

        for (const auto& pos : mesh.positions) {
            shape->addPoint(btVector3(pos.x, pos.y, pos.z));
        }

        return shape;
    }
} // namespace

component create(init_info info, game_entity::entity entity)
{
    assert(entity.is_valid());

    physics_id id{};

    if (free_ids.size() > id::min_deleted_elements)
    {
        id = free_ids.front();
        assert(!exists(id));
        free_ids.pop_front();
        id = physics_id{id::new_generation(id)};
        ++generations[id::index(id)];
    }
    else
    {
        id = physics_id{(id::id_type)id_mapping.size()};
        id_mapping.emplace_back();
        generations.push_back(0);
    }
    assert(id::is_valid(id));
    const id::id_type index{(id::id_type)physics_components.size()};

    // Bullet
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(info.initial_position.x, info.initial_position.y, info.initial_position.z));
    transform.setRotation(btQuaternion(info.initial_orientation.x, info.initial_orientation.y,
                                       info.initial_orientation.z, info.initial_orientation.w));

    auto* motionState = new btDefaultMotionState(transform);

    // Get collision shape
    btCollisionShape* shape = nullptr;
    if (info.scene && !info.scene->lod_groups.empty()) {
        shape = extract_shape(info.scene->lod_groups[0]);
    } else {
        // Default box shape
        shape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
    }

    btVector3 inertia(info.inertia.x, info.inertia.y, info.inertia.z);
    if (!info.is_kinematic && info.mass > 0) {
        shape->calculateLocalInertia(info.mass, inertia);
    }

    btRigidBody::btRigidBodyConstructionInfo rbInfo(
        info.is_kinematic ? 0 : info.mass,
        motionState,
        shape,
        inertia
    );

    auto* rigid_body = new btRigidBody(rbInfo);
    rigid_body->setUserPointer(reinterpret_cast<void*>(entity.get_id()));

    if (info.is_kinematic) {
        rigid_body->setCollisionFlags(rigid_body->getCollisionFlags() |
                                      btCollisionObject::CF_KINEMATIC_OBJECT);
    }

    physics_components.emplace_back(physics_data{
        true,
        rigid_body,
        info.mass
    });

    // Event Physics Object was created
    PhysicObjectCreated event;
    event.body = rigid_body;
    PhysicEventBus::Get().Publish(event);

    id_mapping[id::index(id)] = index;
    return component{id};
}

void remove(component c)
{
    if (!c.is_valid() || !exists(c.get_id()))
        return;

    const physics_id id{c.get_id()};
    const id::id_type index{id_mapping[id::index(id)]};

    // Bullet Cleanup
    auto& data = physics_components[index];
    if (data.body)
    {
        PhysicObjectRemoved event;
        event.body = data.body;
        PhysicEventBus::Get().Publish(event);

        // Delete motion state
        if (data.body->getMotionState())
        {
            delete data.body->getMotionState();
        }

        // Delete collision shape
        if (data.body->getCollisionShape())
        {
            delete data.body->getCollisionShape();
        }

        delete data.body;
        data.body = nullptr;
    }

    const id::id_type last_index{(id::id_type)physics_components.size() - 1};

    if (index != last_index)
    {
        physics_components[index] = std::move(physics_components[last_index]);
        const auto moved_id =
            std::find_if(id_mapping.begin(), id_mapping.end(),
                         [last_index](id::id_type mapping) { return mapping == last_index; });
        if (moved_id != id_mapping.end())
        {
            *moved_id = index;
        }
    }

    physics_components.pop_back();
    id_mapping[id::index(id)] = id::invalid_id;

    if (generations[id::index(id)] < id::max_generation)
    {
        free_ids.push_back(id);
    }
}

void component::apply_force(const math::v3& force, const math::v3& position)
{
    assert(is_valid() && exists(_id));
    auto& data = physics_components[id_mapping[id::index(_id)]];
    if (data.body)
    {
        btVector3 btForce(force.x, force.y, force.z);
        if (position.x == 0 && position.y == 0 && position.z == 0)
        {
            data.body->applyCentralForce(btForce);
        }
        else
        {
            btVector3 btPos(position.x, position.y, position.z);
            data.body->applyForce(btForce, btPos);
        }
    }
}

void component::apply_torque(const math::v3& torque)
{
    assert(is_valid() && exists(_id));
    auto& data = physics_components[id_mapping[id::index(_id)]];
    if (data.body)
    {
        data.body->applyTorque(btVector3(torque.x, torque.y, torque.z));
    }
}

void component::get_state(math::v3& position, math::v4& orientation,
                      math::v3& velocity, math::v3& angular_velocity) const
{
    assert(is_valid() && exists(_id));
    auto& data = physics_components[id_mapping[id::index(_id)]];
    if (data.body)
    {
        const btTransform& transform = data.body->getWorldTransform();
        const btVector3& pos = transform.getOrigin();
        const btQuaternion& rot = transform.getRotation();
        const btVector3& vel = data.body->getLinearVelocity();
        const btVector3& angVel = data.body->getAngularVelocity();

        position = math::v3(pos.x(), pos.y(), pos.z());
        orientation = math::v4(rot.x(), rot.y(), rot.z(), rot.w());
        velocity = math::v3(vel.x(), vel.y(), vel.z());
        angular_velocity = math::v3(angVel.x(), angVel.y(), angVel.z());
    }
}

btRigidBody* component::get_rigid_body() const
{
    assert(is_valid() && exists(_id));
    return physics_components[id_mapping[id::index(_id)]].body;
}

void shutdown()
{
    for (auto& data : physics_components)
    {
        if (data.body)
        {
            if (data.body->getMotionState())
                delete data.body->getMotionState();
            if (data.body->getCollisionShape())
                delete data.body->getCollisionShape();
            delete data.body;
        }
    }

    physics_components.clear();
    id_mapping.clear();
    generations.clear();
    free_ids.clear();
}
} // namespace lark::physics
