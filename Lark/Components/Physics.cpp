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
    drones::Multirotor vehicle;
    drones::Control control;
    std::shared_ptr<drones::Trajectory> trajectory;

    drones::DroneState state;
    drones::ControlInput last_control;

    // Bullet integration
    btRigidBody *body{nullptr};

    // add maybe vector of states for later
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

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(info.state.position.x(), info.state.position.y(), info.state.position.z()));
    float mass = info.params.inertia_properties.mass;
    auto* motionState = new btDefaultMotionState(transform);

    btVector3 inertia(0,0,0);
    auto shape = extract_shape(info.scene->lod_groups[0]);

    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, shape, inertia);
    auto* rigid_body = new btRigidBody(rbInfo);

    rigid_body->setUserPointer(reinterpret_cast<void*>(entity.get_id()));

    physics_components.emplace_back(physics_data{
        true,
        drones::Multirotor(info.params, info.state, info.abstraction),
        info.control,
        std::move(info.trajectory),
        info.state,
        info.last_control,
        rigid_body
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

void component::step(float dt, Eigen::Vector3f wind)
{
    assert(is_valid() && exists(_id));
    auto &data = physics_components[id_mapping[id::index(_id)]];

    // Wind from World
    data.state.wind = wind;

    // Trajectory
    drones::TrajectoryPoint point = data.trajectory->update(dt);

    // Controller
    data.last_control = data.control.computeMotorCommands(data.state, point);

    // Vehicle Step
    data.state = data.vehicle.step(data.state, data.last_control, dt);

    // contains torque and centeral force we give to bullet
    auto [Mtot, Ftot] = data.vehicle.GetPairs();

    data.body->applyCentralForce(btVector3(Ftot.x(), Ftot.y(), Ftot.z()));
    data.body->applyTorque(btVector3(Mtot.x(), Mtot.y(), Mtot.z()));

    // Other Captures here
}

btRigidBody &component::get_rigid_body() const
{
    assert(is_valid());
    assert(exists(_id));
    auto &data = physics_components[id_mapping[id::index(_id)]];

    return *data.body;
}

drones::DroneState component::get_drone_state()
{
    assert(is_valid());
    const id::id_type index = id::index(_id);
    auto &data = physics_components[id_mapping[id::index(_id)]];

    return data.state;
}

btRigidBody * component::try_get_rigid_body() const
{
    if (!is_valid() || !exists(_id))
        return nullptr;

    auto &data = physics_components[id_mapping[id::index(_id)]];
    return data.body;
}

bool component::has_rigid_body() const
{
    if (!is_valid() || !exists(_id))
        return false;

    auto &data = physics_components[id_mapping[id::index(_id)]];
    return data.body != nullptr;
}

void component::set_drone_state(drones::DroneState state)
{
    assert(is_valid());
     const id::id_type index = id::index(_id);
    auto &data = physics_components[id_mapping[id::index(_id)]];
    data.state = std::move(state);
}
} // namespace lark::physics
