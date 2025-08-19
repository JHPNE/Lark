// Tests/PhysicsTests/IntegrationTest.h
#pragma once
#include <gtest/gtest.h>
#include "Components/Entity.h"
#include "Components/Transform.h"
#include "Components/Geometry.h"
#include "Components/Physics.h"
#include "Core/GameLoop.h"
#include "Physics/Environment.h"
#include "Structures/GeometryStructures.h"
#include "APIs/GeometryAPI.h"

namespace lark::test {
    class PhysicsIntegrationTest : public ::testing::Test {
    protected:
        void SetUp() override {
            // Initialize physics world
            physics::Settings settings;
            settings.gravity = glm::vec3(0.0f, 0.0f, -9.81f);
            settings.air_density = 1.225f;
            physics::Environment::getInstance().initialize(settings);

            // Initialize game loop
            GameLoop::Config config;
            config.target_fps = 100;
            config.fixed_timestep = 0.01f;
            game_loop = std::make_unique<GameLoop>(config);
            game_loop->initialize();
        }

        void TearDown() override {
            // Clean up entities
            for (auto id : created_entities) {
                if (game_entity::is_alive(id)) {
                    game_entity::remove(id);
                }
            }
            created_entities.clear();

            game_loop->shutdown();
            physics::Environment::getInstance().shutdown();
        }

        game_entity::entity_id createDroneEntity(
        const glm::vec3& position,
        float mass = 1.0f,
        float arm_length = 0.25f) {

            // Transform
            transform::init_info transform_info{};
            transform_info.position[0] = position.x;
            transform_info.position[1] = position.y;
            transform_info.position[2] = position.z;
            transform_info.rotation[0] = 0;
            transform_info.rotation[1] = 0;
            transform_info.rotation[2] = 0;
            transform_info.rotation[3] = 1;
            transform_info.scale[0] = transform_info.scale[1] = transform_info.scale[2] = 1.0f;

            // Convert packed data to scene
            auto scene = std::make_shared<tools::scene>();
            // Note: You'd need to unpack the scene_data here
            // For simplicity, creating an empty scene
            tools::lod_group lod;
            lod.name = "drone";
            scene->lod_groups.push_back(lod);

            geometry::init_info geometry_info{};
            geometry_info.scene = scene;
            geometry_info.is_dynamic = false;

            // Physics - quadrotor configuration
            physics::init_info physics_info{};

            physics_info.inertia.mass = mass;
            physics_info.inertia.Ixx = mass * arm_length * arm_length * 0.01f;
            physics_info.inertia.Iyy = physics_info.inertia.Ixx;
            physics_info.inertia.Izz = physics_info.inertia.Ixx * 2.0f;
            physics_info.inertia.Ixy = physics_info.inertia.Iyz = physics_info.inertia.Ixz = 0.0f;

            physics_info.aerodynamic.dragCoeffX = 0.1f;
            physics_info.aerodynamic.dragCoeffY = 0.1f;
            physics_info.aerodynamic.dragCoeffZ = 0.1f;
            physics_info.aerodynamic.enableAerodynamics = true;

            physics_info.motor.responseTime = 0.02f;
            physics_info.motor.noiseStdDev = 0.0f;
            physics_info.motor.bodyRateGain = 5.0f;
            physics_info.motor.velocityGain = 2.5f;
            physics_info.motor.attitudePGain = 50.0f;
            physics_info.motor.attitudeDGain = 10.0f;

            // X configuration quadrotor
            float offset = arm_length / std::sqrt(2.0f);
            physics_info.rotors = {
                {5.57e-6f, 1.36e-7f, 1e-4f, 1e-4f, 1e-5f,  // More realistic thrust coefficient
                 glm::vec3(offset, offset, 0), 1, 0, 1500},
                {5.57e-6f, 1.36e-7f, 1e-4f, 1e-4f, 1e-5f,
                 glm::vec3(offset, -offset, 0), -1, 0, 1500},
                {5.57e-6f, 1.36e-7f, 1e-4f, 1e-4f, 1e-5f,
                 glm::vec3(-offset, -offset, 0), 1, 0, 1500},
                {5.57e-6f, 1.36e-7f, 1e-4f, 1e-4f, 1e-5f,
                 glm::vec3(-offset, offset, 0), -1, 0, 1500}
            };

            physics_info.control_mode = drones::ControlMode::COLLECTIVE_THRUST_ATTITUDE;

            // Create entity
            game_entity::entity_info entity_info{};
            entity_info.transform = &transform_info;
            entity_info.geometry = &geometry_info;
            entity_info.physics = &physics_info;

            auto entity = game_entity::create(entity_info);
            created_entities.push_back(entity.get_id());

            return entity.get_id();
        }

        std::unique_ptr<GameLoop> game_loop;
        std::vector<game_entity::entity_id> created_entities;
    };

    TEST_F(PhysicsIntegrationTest, DroneEntityCreation) {
    auto drone_id = createDroneEntity(glm::vec3(0, 0, 1));

    ASSERT_TRUE(game_entity::is_alive(drone_id));

    game_entity::entity entity{drone_id};
    EXPECT_TRUE(entity.transform().is_valid());
    EXPECT_TRUE(entity.geometry().is_valid());
    EXPECT_TRUE(entity.physics().is_valid());
    }

    TEST_F(PhysicsIntegrationTest, HoverSimulation) {
        auto drone_id = createDroneEntity(glm::vec3(0, 0, 0));
        game_entity::entity entity{drone_id};

        // Set hover trajectory at 1 meter
        auto physics = entity.physics();
        ASSERT_TRUE(physics.is_valid());

        auto hover_trajectory = std::make_shared<physics::trajectory::HoverTrajectory>(
            glm::vec3(0, 0, 1), 0.0f
        );
        physics.set_trajectory(hover_trajectory);

        // Simulate for 3 seconds
        const int steps = 500;

        for (int i = 0; i < steps; ++i) {
            game_loop->tick();
        }

        // Check final position
        auto transform = entity.transform();
        auto final_position = transform.position();

        EXPECT_NEAR(final_position.x, 0.0f, 0.1f);
        EXPECT_NEAR(final_position.y, 0.0f, 0.1f);
        EXPECT_NEAR(final_position.z, 1.0f, 0.3f); // Should be close to 1 meter
    }

    TEST_F(PhysicsIntegrationTest, CircularTrajectory) {
        auto drone_id = createDroneEntity(glm::vec3(0, 0, 1));
        game_entity::entity entity{drone_id};

        auto physics = entity.physics();
        ASSERT_TRUE(physics.is_valid());

        // Set circular trajectory
        physics::trajectory::CircularTrajectory::Parameters params;
        params.center = glm::vec3(0, 0, 1);
        params.radius = 2.0f;
        params.height = 1.0f;
        params.frequency = 0.1f; // Slow rotation
        params.yaw_follows_velocity = true;

        auto circular_trajectory = std::make_shared<physics::trajectory::CircularTrajectory>(params);
        physics.set_trajectory(circular_trajectory);

        // Simulate for 5 seconds
        const int steps = 500;

        glm::vec3 last_position = entity.transform().position();
        bool is_moving = false;

        for (int i = 0; i < steps; ++i) {
            game_loop->tick();

            auto current_position = entity.transform().position();
            float distance_moved = glm::length(current_position - last_position);

            if (distance_moved > 0.001f) {
                is_moving = true;
            }

            last_position = current_position;
        }

        EXPECT_TRUE(is_moving) << "Drone should be moving in circular trajectory";

        // Check that drone is approximately at the expected radius
        auto final_position = entity.transform().position();
        float horizontal_distance = glm::length(glm::vec2(final_position.x, final_position.y));
        EXPECT_NEAR(horizontal_distance, 2.0f, 0.5f); // Should be close to radius
    }

    TEST_F(PhysicsIntegrationTest, MultiDroneSimulation) {
        // Create multiple drones
        auto drone1 = createDroneEntity(glm::vec3(-2, 0, 1));
        auto drone2 = createDroneEntity(glm::vec3(0, 0, 1));
        auto drone3 = createDroneEntity(glm::vec3(2, 0, 1));

        // Set different trajectories
        game_entity::entity{drone1}.physics().set_trajectory(
            std::make_shared<physics::trajectory::HoverTrajectory>(glm::vec3(-2, 0, 2), 0.0f)
        );

        game_entity::entity{drone2}.physics().set_trajectory(
            std::make_shared<physics::trajectory::HoverTrajectory>(glm::vec3(0, 0, 3), 0.0f)
        );

        game_entity::entity{drone3}.physics().set_trajectory(
            std::make_shared<physics::trajectory::HoverTrajectory>(glm::vec3(2, 0, 2), 0.0f)
        );

        // Simulate
        const int steps = 300;
        for (int i = 0; i < steps; ++i) {
            game_loop->tick();
        }

        // Check all drones are at different positions
        auto pos1 = game_entity::entity{drone1}.transform().position();
        auto pos2 = game_entity::entity{drone2}.transform().position();
        auto pos3 = game_entity::entity{drone3}.transform().position();

        float dist12 = glm::length(pos1 - pos2);
        float dist23 = glm::length(pos2 - pos3);
        float dist13 = glm::length(pos1 - pos3);

        EXPECT_GT(dist12, 1.0f) << "Drones should maintain separation";
        EXPECT_GT(dist23, 1.0f) << "Drones should maintain separation";
        EXPECT_GT(dist13, 1.0f) << "Drones should maintain separation";
    }

}
