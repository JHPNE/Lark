#include "Physics.h"

#include <utility>

#include "Physics/Controller.h"
#include "Physics/Environment.h"
#include "Physics/Multirotor.h"

namespace lark::physics {
    namespace {
        struct physics_data {
            std::unique_ptr<drones::IDrone> drone;
            std::unique_ptr<drones::Controller> controller;
            std::shared_ptr<trajectory::ITrajectory> trajectory;
            drones::DroneState current_state;
            drones::ControlInput current_control;
            game_entity::entity_id entity_id;
            float trajectory_time{0.0f};
            float control_update_time{0.0f};
            float control_rate{100.0f}; // Hz
            bool is_valid{false};
            float simulation_time{0.0f};
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

        // Ensure physics world is initialized
        if (!Environment::getInstance().isInitialized()) {
            Environment::getInstance().initialize();
        }

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

        drones::ControllerGains gains;
        gains.kPosition = glm::vec3(3.0f, 3.0f, 6.0f);  // Reduced from {6.5f, 6.5f, 15.0f}
        gains.kVelocity = glm::vec3(2.0f, 2.0f, 4.0f);  // Reduced from {4.0f, 4.0f, 9.0f}
        gains.kAttitudeP = 200.0f;  // Reduced from 544.0f
        gains.kAttitudeD = 20.0f;   // Reduced from 46.64f
        gains.kVelocityP = 0.5f;    // Reduced from 0.65f

        // Create controller
        auto controller = std::make_unique<drones::Controller>(
            info.inertia,
            gains
        );

        // Initialize state
        drones::DroneState initial_state;
        auto transform = entity.transform();

        if (transform.is_valid()) {
            initial_state.position = transform.position();

            math::v4 rot = transform.rotation();
            initial_state.orientation = glm::quat(rot.w, rot.x, rot.y, rot.z);

            initial_state.velocity = glm::vec3(0.0f);
            initial_state.angular_velocity = glm::vec3(0.0f);
            initial_state.wind = glm::vec3(0.0f);
            initial_state.rotor_speeds.resize(info.rotors.size(), 0.0f);
        }

        physics_components.emplace_back(physics_data{
            std::move(drone),
            std::move(controller),
            nullptr, // No trajectory initially
            initial_state,
            drones::ControlInput{},
            entity.get_id(),
            0.0f,
            0.0f,
            100.0f,
            true,
            0.0f
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

        if (!data.drone || !data.is_valid) return;

        // Get world settings
        const auto& world = Environment::getInstance();
        const auto& settings = world.getSettings();

        // Update wind
        data.current_state.wind = world.getWindAt(data.simulation_time, data.current_state.position);

        // Update control if we have a trajectory
        data.control_update_time += dt;

        if (data.trajectory && data.control_update_time >= 1.0f / data.control_rate) {
            data.trajectory_time += data.control_update_time;
            data.control_update_time = 0.0f;

            auto flat_output = data.trajectory->update(data.trajectory_time);

            // Check if trajectory is complete
            if (data.trajectory->isComplete(data.trajectory_time)) {
                // Hold last position
                flat_output.velocity = glm::vec3(0.0f);
                flat_output.acceleration = glm::vec3(0.0f);
            }

            data.current_control = data.controller->computeControl(
                data.drone->getControlMode(),
                data.current_state,
                flat_output
            );
        }

        // Step the drone simulation
        data.current_state = data.drone->step(
            data.current_state,
            data.current_control,
            dt
        );

        // Update entity transform
        game_entity::entity entity{data.entity_id};
        auto transform = entity.transform();
        if (transform.is_valid()) {
            transform.set_position(data.current_state.position);
            // TODO: use helper or something
            // Maintain quaternion order (x, y, z, w) for vec4
            transform.set_rotation(math::v4(
                data.current_state.orientation.x,
                data.current_state.orientation.y,
                data.current_state.orientation.z,
                data.current_state.orientation.w
            ));
        }

        data.simulation_time += dt;
    }

    void component::set_control_input(const drones::ControlInput& input) {
        assert(is_valid() && exists(_id));
        physics_components[id_mapping[id::index(_id)]].current_control = input;
    }

    drones::DroneState component::get_state() const {
        assert(is_valid() && exists(_id));
        return physics_components[id_mapping[id::index(_id)]].current_state;
    }

    void component::set_trajectory(std::shared_ptr<trajectory::ITrajectory> trajectory) {
        assert(is_valid() && exists(_id));
        auto& data = physics_components[id_mapping[id::index(_id)]];
        data.trajectory = std::move(trajectory);
        data.trajectory_time = 0.0f;
    }

    void component::set_controller_gains(const drones::ControllerGains& gains) {
        assert(is_valid() && exists(_id));
        auto& data = physics_components[id_mapping[id::index(_id)]];
        data.controller = std::make_unique<drones::Controller>(
            data.drone->getInertialProperties(),
            gains
        );
    }

    void component::set_control_mode(drones::ControlMode mode) {
        assert(is_valid() && exists(_id));
        physics_components[id_mapping[id::index(_id)]].drone->setControlMode(mode);
    }

    void shutdown() {
        physics_components.clear();
        id_mapping.clear();
        generations.clear();
        free_ids.clear();
        Environment::getInstance().shutdown();
    }
}