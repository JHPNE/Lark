#pragma once
#include "DroneExtension/Components/Fuselage.h"
#include "DroneExtension/Components/Rotor.h"
#include "DroneExtension/DroneManager.h"
#include "DronePhysicsRenderer.h"

#include <btBulletDynamicsCommon.h>
#include <glm/gtx/string_cast.hpp>
#include <iostream>
#include <iomanip>

namespace lark::physics {

struct TransformTestConfig {
    bool visual_mode{false};
    float test_duration{5.0f};
};

class TransformationTest {
public:
    explicit TransformationTest(const TransformTestConfig& config = TransformTestConfig{})
        : m_config(config),
          m_renderer(config.visual_mode ? new visualization::DronePhysicsRenderer(1280, 720) : nullptr) {
        initializePhysics();
        setupDrone();
    }

    void run() {
        std::cout << "Starting Transform Tests\n" << std::endl;

        // Test 1: Initial positions
        testInitialPositions();

        // Test 2: Rotation around Y axis
        testRotation();

        // Test 3: Translation
        testTranslation();

        // Test 4: Combined transform
        testCombinedTransform();

        if (m_config.visual_mode) {
            runVisualMode();
        }
    }

private:
    TransformTestConfig m_config;
    std::unique_ptr<visualization::DronePhysicsRenderer> m_renderer;
    std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfiguration;
    std::unique_ptr<btCollisionDispatcher> m_dispatcher;
    std::unique_ptr<btBroadphaseInterface> m_broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_dynamicsWorld;
    drone_entity::entity m_drone;

    void initializePhysics() {
        m_collisionConfiguration = std::make_unique<btDefaultCollisionConfiguration>();
        m_dispatcher = std::make_unique<btCollisionDispatcher>(m_collisionConfiguration.get());
        m_broadphase = std::make_unique<btDbvtBroadphase>();
        m_solver = std::make_unique<btSequentialImpulseConstraintSolver>();
        m_dynamicsWorld = std::make_unique<btDiscreteDynamicsWorld>(
            m_dispatcher.get(),
            m_broadphase.get(),
            m_solver.get(),
            m_collisionConfiguration.get()
        );
        m_dynamicsWorld->setGravity(btVector3(0, 0, 0)); // Disable gravity for transform tests
    }

    void setupDrone() {
        fuselage::init_info fuselageInfo;
        // Move the fuselage up by 1 unit to be above the ground
        fuselageInfo.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        fuselageInfo.shape.type = drone_data::ComponentShape::ShapeTypes::BOX;
        fuselageInfo.shape.dimensions = glm::vec3(0.5f, 0.2f, 0.5f);  // Main body size

        // Create 4 rotors in X formation at same height as fuselage
        std::vector<rotor::init_info> rotorInfos(4);
        glm::vec3 rotorPositions[4] = {
            {1.0f, 1.0f, 1.0f},   // Front Right
            {1.0f, 1.0f, -1.0f},  // Back Right
            {-1.0f, 1.0f, 1.0f},  // Front Left
            {-1.0f, 1.0f, -1.0f}  // Back Left
        };

        for (int i = 0; i < 4; i++) {
            rotorInfos[i].transform = glm::translate(glm::mat4(1.0f), rotorPositions[i]);
            rotorInfos[i].bladeRadius = 0.2f;
            rotorInfos[i].mass = 0.1f;
            rotorInfos[i].shape.type = drone_data::ComponentShape::ShapeTypes::BOX;
            rotorInfos[i].shape.dimensions = glm::vec3(0.2f, 0.05f, 0.2f);  // Rotor size
        }

        // Create drone entity
        drone_entity::entity_info info{};
        info.fuselage = &fuselageInfo;
        for (auto& rotorInfo : rotorInfos) {
            info.rotors.push_back(&rotorInfo);
        }

        m_drone = drone_entity::create(info);
        assert(m_drone.is_valid());
    }

    void renderFrame() {
        if (!m_renderer) return;

        m_renderer->clear();  // Clear previous objects

        // Add fuselage
        auto fuselage_component = m_drone.fuselage();
        if (fuselage_component.is_valid()) {
            auto transform = fuselage::get_transform(fuselage_component);
            m_renderer->setCameraTarget(glm::vec3(transform[3]));
            m_renderer->addObject(transform, glm::vec3(0.3f, 0.3f, 0.3f), glm::vec3(0.5f, 0.2f, 0.5f));
        }

        // Add rotors
        auto rotors = m_drone.rotor();
        const glm::vec3 rotor_colors[4] = {
            {0.8f, 0.2f, 0.2f},  // Red
            {0.2f, 0.8f, 0.2f},  // Green
            {0.2f, 0.2f, 0.8f},  // Blue
            {0.8f, 0.8f, 0.2f}   // Yellow
        };

        for (size_t i = 0; i < rotors.size(); ++i) {
            if (rotors[i].is_valid()) {
                auto transform = rotor::get_transform(rotors[i]);
                m_renderer->addObject(transform, rotor_colors[i], glm::vec3(0.2f, 0.05f, 0.2f));
            }
        }

        m_renderer->render();
    }

    void testInitialPositions() {
        std::cout << "Test 1: Verifying Initial Positions" << std::endl;

        auto rotors = m_drone.rotor();
        for (size_t i = 0; i < rotors.size(); ++i) {
            auto transform = rotor::get_transform(rotors[i]);
            glm::vec3 position(transform[3]);
            std::cout << "Rotor " << i << " position: " << glm::to_string(position) << std::endl;
        }
        std::cout << std::endl;

        if (m_config.visual_mode) {
            std::cout << "Rendering initial positions for 2 seconds..." << std::endl;
            float time = 0.0f;
            while (time < 2.0f && !m_renderer->shouldClose()) {
                renderFrame();
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                time += 0.016f;
            }
        }
    }

    void testRotation() {
        std::cout << "Test 2: Testing 90-degree Y-axis Rotation" << std::endl;

        auto rotors = m_drone.rotor();
        std::vector<glm::vec3> initial_positions;
        for (const auto& rotor : rotors) {
            auto transform = rotor::get_transform(rotor);
            initial_positions.push_back(glm::vec3(transform[3]));
        }

        if (m_config.visual_mode) {
            std::cout << "Performing rotation animation..." << std::endl;
            float rotation = 0.0f;
            while (rotation < 90.0f && !m_renderer->shouldClose()) {
                float delta_rotation = 2.0f;  // degrees per frame
                glm::mat4 rot = glm::rotate(glm::mat4(1.0f), glm::radians(delta_rotation), glm::vec3(0, 1, 0));
                auto test = rot * fuselage::get_transform(m_drone.fuselage());
                drone_entity::transform(m_drone.get_id(), test);

                renderFrame();
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                rotation += delta_rotation;
            }
        } else {
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0, 1, 0));
            drone_entity::transform(m_drone.get_id(), rotation);
        }

        // Verify new positions
        for (size_t i = 0; i < rotors.size(); ++i) {
            auto transform = rotor::get_transform(rotors[i]);
            glm::vec3 new_position(transform[3]);
            std::cout << "Rotor " << i << ":\n"
                     << "  Initial: " << glm::to_string(initial_positions[i]) << "\n"
                     << "  After rotation: " << glm::to_string(new_position) << std::endl;
        }
        std::cout << std::endl;

        if (m_config.visual_mode) {
            std::cout << "Holding final rotation position for 2 seconds..." << std::endl;
            float time = 0.0f;
            while (time < 2.0f && !m_renderer->shouldClose()) {
                renderFrame();
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                time += 0.016f;
            }
        }
    }

    void testTranslation() {
        std::cout << "Test 3: Testing Translation" << std::endl;

        auto rotors = m_drone.rotor();
        std::vector<glm::vec3> initial_positions;
        for (const auto& rotor : rotors) {
            auto transform = rotor::get_transform(rotor);
            initial_positions.push_back(glm::vec3(transform[3]));
        }

        if (m_config.visual_mode) {
            std::cout << "Performing translation animation..." << std::endl;
            float height = 0.0f;
            while (height < 2.0f && !m_renderer->shouldClose()) {
                float delta_height = 0.05f;
                glm::mat4 trans = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, delta_height, 0.0f));
                auto test = trans * fuselage::get_transform(m_drone.fuselage());
                drone_entity::transform(m_drone.get_id(), test);

                renderFrame();
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                height += delta_height;
            }
        } else {
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, 0.0f));
            drone_entity::transform(m_drone.get_id(), translation);
        }

        // Verify new positions
        for (size_t i = 0; i < rotors.size(); ++i) {
            auto transform = rotor::get_transform(rotors[i]);
            glm::vec3 new_position(transform[3]);
            std::cout << "Rotor " << i << ":\n"
                     << "  Initial: " << glm::to_string(initial_positions[i]) << "\n"
                     << "  After translation: " << glm::to_string(new_position) << std::endl;
        }
        std::cout << std::endl;
    }

    void testCombinedTransform() {
        std::cout << "Test 4: Testing Combined Rotation and Translation" << std::endl;

        auto rotors = m_drone.rotor();
        std::vector<glm::vec3> initial_positions;
        for (const auto& rotor : rotors) {
            auto transform = rotor::get_transform(rotor);
            initial_positions.push_back(glm::vec3(transform[3]));
        }

        if (m_config.visual_mode) {
            std::cout << "Performing combined transformation animation..." << std::endl;
            float progress = 0.0f;
            while (progress < 1.0f && !m_renderer->shouldClose()) {
                float delta = 0.02f;
                glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f * delta), glm::vec3(0, 1, 0));
                glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f) * delta);
                auto test = translation * rotation * fuselage::get_transform(m_drone.fuselage());
                drone_entity::transform(m_drone.get_id(), test);

                renderFrame();
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
                progress += delta;
            }
        } else {
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(45.0f), glm::vec3(0, 1, 0));
            glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f));
            glm::mat4 combined = translation * rotation;
            drone_entity::transform(m_drone.get_id(), combined);
        }

        // Verify new positions
        for (size_t i = 0; i < rotors.size(); ++i) {
            auto transform = rotor::get_transform(rotors[i]);
            glm::vec3 new_position(transform[3]);
            std::cout << "Rotor " << i << ":\n"
                     << "  Initial: " << glm::to_string(initial_positions[i]) << "\n"
                     << "  After combined transform: " << glm::to_string(new_position) << std::endl;
        }
        std::cout << std::endl;
    }

    void runVisualMode() {
        float time = 0.0f;
        while (!m_renderer->shouldClose() && time < m_config.test_duration) {
            renderFrame();
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            time += 0.016f;
        }
    }
};

} // namespace lark::physics