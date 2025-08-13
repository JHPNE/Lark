#pragma once
#include "IDrone.h"
#include "Controller.h"
#include "TrajectorySystem.h"
#include "Wind.h"
#include "../Components/Entity.h"
#include <memory>
#include <chrono>

namespace lark::physics {
    class Environment {
        public:
            struct Config {
                float simulation_rate{500.0f};
                float control_rate{100.0f};
                glm::vec3 gravity{0.0f, 0.0f, -9.81f};
                float air_density{1.225f};
                bool enable_collisions{true};
                float safety_margin{0.25f};
            };

            struct SimulationResult {
                enum class ExitStatus {
                    SUCCESS,
                    TIMEOUT,
                    COLLISION,
                    OUT_OF_BOUNDS,
                    NUMERICAL_ERROR,
                    CONTROL_FAILURE
                };

                ExitStatus status;
                float final_time;
                std::string message;
            };

            explicit Environment(const Config& config = Config{500.0f, 5.0f,{0.0f, 0.0f, -9.81f} , 5, true, 0.25f});
            ~Environment();

            game_entity::entity_id spawn_drone(
                const drones::InertiaProperties& inertia,
                const drones::AerodynamicProperties& aero,
                const drones::MotorProperties& motor,
                const std::vector<drones::RotorParameters>& rotors,
                const glm::vec3& initial_position = glm::vec3(0.0f),
                const glm::quat& initial_orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f)
            );
            void remove_drone(game_entity::entity_id id);

            // Simulation control
            void set_trajectory(game_entity::entity_id drone,
                               std::shared_ptr<trajectory::ITrajectory> traj);
            void set_wind_profile(std::shared_ptr<wind::IWindProfile> wind);
            void set_controller_gains(game_entity::entity_id drone,
                                     const drones::ControllerGains& gains);

            // Main simulation step
            SimulationResult step(float dt);
            void reset();

            // State queries
            drones::DroneState get_drone_state(game_entity::entity_id id) const;

        private:
            struct DroneInstance {
                game_entity::entity_id entity_id;
                std::unique_ptr<drones::IDrone> drone;
                std::unique_ptr<drones::Controller> controller;
                std::shared_ptr<trajectory::ITrajectory> trajectory;
                drones::DroneState state;
                float trajectory_time{0.0f};
                float control_accumulator{0.0f};
                float sensor_accumulator{0.0f};
            };

            Config config;
            std::vector<std::unique_ptr<DroneInstance>> drones;
            std::shared_ptr<wind::IWindProfile> wind_profile;
            float simulation_time{0.0f};

            // Physics integration
            void update_drone_physics(DroneInstance& drone, float dt);
            void update_drone_control(DroneInstance& drone, float dt);
            void update_drone_sensors(DroneInstance& drone, float dt);
            bool check_collisions(const DroneInstance& drone) const;
            bool check_bounds(const DroneInstance& drone) const;
    };
}