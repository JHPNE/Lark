#include "Physics.h"

#include "btBulletDynamicsCommon.h"
#include "PhysicExtension/Controller/Controller.h"
#include "PhysicExtension/Utils/DroneDynamics.h"
#include "PhysicExtension/Utils/Wind.h"
#include "PhysicExtension/Vehicles/Multirotor.h"

namespace lark::physics {
    namespace {
        struct physics_data {
            game_entity::entity_id entity_id;
            bool is_valid{false};

            lark::drones::Multirotor vehicle;
            lark::drones::Control control;
            std::unique_ptr<lark::drones::Wind> wind;
            std::unique_ptr<lark::drones::Trajectory> trajectory;

            float sim_time{0.f};
            lark::drones::DroneState state;
            lark::drones::ControlInput last_control;

            // Bullet integration
            btRigidBody* body{nullptr};
            std::unique_ptr<btCollisionShape> shape;
            std::unique_ptr<btDefaultMotionState> motion;

            // add maybe vector of states for later
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
        assert(id::is_valid(id));
        const id::id_type index{(id::id_type)physics_components.size()};

        /*
        physics_components.emplace_back(physics_data{
            lark::drones::Multirotor{}
        });
        */



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

        // Time
        data.sim_time += dt;

        // Wind
        Eigen::Vector3f w = data.wind->update(data.sim_time, data.state.position);
        data.state.wind = w;

        // Trajectory
        lark::drones::TrajectoryPoint point = data.trajectory->update(data.sim_time);

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

    btRigidBody& component::get_rigid_body() const {
        assert(is_valid());
        const id::id_type index = id::index(_id);
        auto& data = physics_components[id_mapping[id::index(_id)]];

        return *data.body;
    }


    drones::DroneState component::get_drone_state() {
        assert(is_valid());
        const id::id_type index = id::index(_id);
        auto& data = physics_components[id_mapping[id::index(_id)]];

        return data.state;
    }


}
