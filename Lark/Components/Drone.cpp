#include "Drone.h"
#include <utility>

namespace lark::drone {
    namespace {
        struct drone_data
        {
            bool is_valid{false};
            Multirotor vehicle;
            Control control;
            std::shared_ptr<Trajectory> trajectory;
            DroneState state;
            ControlInput last_control;
        };

        util::vector<drone_data> drone_components;
        util::vector<id::id_type> id_mapping;
        util::vector<id::generation_type> generations;
        std::deque<drone_id> free_ids;

        bool exists(drone_id id)
        {
            assert(id::is_valid(id));
            const id::id_type index{id::index(id)};
            assert(index < generations.size());
            return (id::is_valid(id_mapping[index]) && generations[index] == id::generation(id) &&
                    drone_components[id_mapping[index]].is_valid);
        }

    }

    component create(init_info info, game_entity::entity entity) {
        assert(entity.is_valid());

        drone_id id{};

        if (free_ids.size() > id::min_deleted_elements)
        {
            id = free_ids.front();
            assert(!exists(id));
            free_ids.pop_front();
            id = drone_id{id::new_generation(id)};
            ++generations[id::index(id)];
        }
        else
        {
            id = drone_id{(id::id_type)id_mapping.size()};
            id_mapping.emplace_back();
            generations.push_back(0);
        }

        assert(id::is_valid(id));
        const id::id_type index{(id::id_type)drone_components.size()};

        drone_components.emplace_back(drone_data{
            true,
            Multirotor(info.params, info.initial_state, info.abstraction),
            Control{info.params},
            std::move(info.trajectory),
            info.initial_state,
            info.last_control
        });

        id_mapping[id::index(id)] = index;
        return component{id};

    }

    void remove(component c)
    {
        if (!c.is_valid() || !exists(c.get_id()))
            return;

        const drone_id id{c.get_id()};
        const id::id_type index{id_mapping[id::index(id)]};
        const id::id_type last_index{(id::id_type)drone_components.size() - 1};

        if (index != last_index)
        {
            drone_components[index] = std::move(drone_components[last_index]);
            const auto moved_id =
                std::find_if(id_mapping.begin(), id_mapping.end(),
                             [last_index](id::id_type mapping) { return mapping == last_index; });
            if (moved_id != id_mapping.end())
            {
                *moved_id = index;
            }
        }

        drone_components.pop_back();
        id_mapping[id::index(id)] = id::invalid_id;

        if (generations[id::index(id)] < id::max_generation)
        {
            free_ids.push_back(id);
        }
    }

    void component::update(float dt, const Eigen::Vector3f& wind)
    {
        assert(is_valid() && exists(_id));
        auto &data = drone_components[id_mapping[id::index(_id)]];

        // Update wind
        data.state.wind = wind;

        // Update trajectory
        TrajectoryPoint point = data.trajectory->update(dt);

        // Compute control
        data.last_control = data.control.computeMotorCommands(data.state, point);

        // Vehicle dynamics step
        data.state = data.vehicle.step(data.state, data.last_control, dt);
    }

    std::pair<Eigen::Vector3f, Eigen::Vector3f> component::get_forces_and_torques() const
    {
        assert(is_valid() && exists(_id));
        auto &data = drone_components[id_mapping[id::index(_id)]];
        return data.vehicle.GetPairs();
    }

    DroneState component::get_state() const
    {
        assert(is_valid() && exists(_id));
        return drone_components[id_mapping[id::index(_id)]].state;
    }

    void component::set_state(const DroneState& state)
    {
        assert(is_valid() && exists(_id));
        drone_components[id_mapping[id::index(_id)]].state = state;
    }

    void component::sync_from_physics(const math::v3& position, const math::v4& orientation,
                                  const math::v3& velocity, const math::v3& angular_velocity)
    {
        assert(is_valid() && exists(_id));
        auto &data = drone_components[id_mapping[id::index(_id)]];

        data.state.position = Eigen::Vector3f(position.x, position.y, position.z);
        data.state.attitude = Eigen::Vector4f(orientation.x, orientation.y, orientation.z, orientation.w);
        data.state.velocity = Eigen::Vector3f(velocity.x, velocity.y, velocity.z);
        data.state.body_rates = Eigen::Vector3f(angular_velocity.x, angular_velocity.y, angular_velocity.z);
    }

    void shutdown()
    {
        drone_components.clear();
        id_mapping.clear();
        generations.clear();
        free_ids.clear();
    }
}