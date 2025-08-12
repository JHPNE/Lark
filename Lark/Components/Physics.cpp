#include "Physics.h"
#include "Physics/Multirotor.h"

namespace lark::physics {
    namespace {
        struct physics_data {
            std::unique_ptr<drones::IDrone> drone;
            drones::DroneState current_state;
            drones::ControlInput current_control;
            game_entity::entity_id entity_id;
            bool is_valid{false};
        };

        util::vector<physics_data> physics_components;
        util::vector<id::id_type> id_mapping;
        util::vector<id::generation_type> generations;
        std::deque<physics_id> free_ids;

        bool exists(physics_id id) {
            assert(id::is_valid(id));
            const id::id_type index{id::index(id)};
            assert(index < generations.size());
            return (id::is_valid(id_mapping[index]) &&
                generations[index] == id::generation(id) &&
                physics_components[id_mapping[index]].is_valid);
        }
    }

    component create(init_info info, game_entity::entity entity) {
        assert(entity.is_valid());

        physics_id id{};

        if (free_ids.size() > id::min_deleted_elements) {
            id = free_ids.front();
            free_ids.pop_front();
            id = physics_id{id::new_generation(id)};
            ++generations[id::index(id)];
        } else {
            id = physics_id{(id::id_type)id_mapping.size()};
            id_mapping.emplace_back();
            generations.push_back(0);
        }

        const id::id_type index{(id::id_type)physics_components.size()};

        // Create drone instance
        auto drone = std::make_unique<drones::Multirotor>(
            info.inertia,
            info.aerodynamic,
            info.motor,
            info.rotors,
            info.control_mode
        );

        // Initialize state
        drones::DroneState initial_state;
        auto transform = entity.transform();
        initial_state.position = transform.position();
        initial_state.velocity = glm::vec3(0.0f);
        initial_state.orientation = glm::quat(transform.rotation());
        initial_state.angular_velocity = glm::vec3(0.0f);
        initial_state.wind = glm::vec3(0.0f);
        initial_state.rotor_speeds.resize(info.rotors.size(), 0.0f);

        physics_components.emplace_back(physics_data{
            std::move(drone),
            initial_state,
            drones::ControlInput{},
            entity.get_id(),
            true
        });

        id_mapping[id::index(id)] = index;
        return component{id};
    }

    void remove(component c) {
        if (!c.is_valid() || !exists(c.get_id())) return;

        const physics_id id{c.get_id()};
        const id::id_type index{id_mapping[id::index(id)]};
        const id::id_type last_index{(id::id_type)physics_components.size() - 1};

        if (index != last_index) {
            physics_components[index] = std::move(physics_components[last_index]);
            const auto moved_id = std::find_if(id_mapping.begin(), id_mapping.end(),
                [last_index](id::id_type mapping) { return mapping == last_index; });
            if (moved_id != id_mapping.end()) {
                *moved_id = index;
            }
        }

        physics_components.pop_back();
        id_mapping[id::index(id)] = id::invalid_id;

        if (generations[id::index(id)] < id::max_generation) {
            free_ids.push_back(id);
        }
    }

    void component::step(float dt) {
        assert(is_valid() && exists(_id));
        auto& data = physics_components[id_mapping[id::index(_id)]];

        if (data.drone && data.is_valid) {
            data.current_state = data.drone->step(
                data.current_state,
                data.current_control,
                dt
            );
        }
    }

    void component::set_control_input(const drones::ControlInput& input) {
        assert(is_valid() && exists(_id));
        physics_components[id_mapping[id::index(_id)]].current_control = input;
    }

    drones::DroneState component::get_state() const {
        assert(is_valid() && exists(_id));
        return physics_components[id_mapping[id::index(_id)]].current_state;
    }

    void component::apply_wind(const glm::vec3& wind) {
        assert(is_valid() && exists(_id));
        physics_components[id_mapping[id::index(_id)]].current_state.wind = wind;
    }

    void shutdown() {
        physics_components.clear();
        id_mapping.clear();
        generations.clear();
        free_ids.clear();
    }
}

