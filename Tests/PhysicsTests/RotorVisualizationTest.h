#pragma once
#include "DroneExtension/Components/Fuselage.h"
#include "DroneExtension/Components/Rotor.h"
#include "DroneExtension/DroneManager.h"
#include "DronePhysicsRenderer.h"

#include <btBulletDynamicsCommon.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <memory>
#include <thread>

namespace lark::physics {

struct RotorTestConfig {
    bool visual_mode{false};
    float simulation_speed{10.0f};
    float target_rpm{5000.0f};
    float test_duration{6000.0f};
    bool ground_effect{false};
};

class RotorVisualizationTest {
public:
    explicit RotorVisualizationTest(const RotorTestConfig& config = RotorTestConfig{})
        : m_config(config),
          m_renderer(config.visual_mode ? new visualization::DronePhysicsRenderer(1280, 720) : nullptr) {
        initializePhysics();
        setupRotor();
    }

    void run() {
        if (m_config.ground_effect) {
            runGroundEffectTest();
            return;
        }
        const float base_timestep = 1.0f / 60.0f;
        const float simulation_timestep = base_timestep * m_config.simulation_speed;
        auto lastTime = std::chrono::high_resolution_clock::now();
        float testTime = 0.0f;

        m_rotorComponent[0].set_rpm(m_config.target_rpm);

        logConfiguration();

        while (shouldContinue(testTime)) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = m_config.visual_mode ?
                std::chrono::duration<float>(currentTime - lastTime).count() :
                simulation_timestep;

            lastTime = currentTime;
            testTime += deltaTime;

            updatePhysics(deltaTime);

            m_timeSinceLastLog += deltaTime;
            if (m_timeSinceLastLog >= (m_config.visual_mode ? 1.0f : 0.1f)) {
                logState(testTime);
                m_timeSinceLastLog = 0.0f;
            }

            if (m_config.visual_mode && m_renderer) {
                renderFrame();
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }
        }
    }

    void clearTrajectory() {
        m_trajectoryPoints.clear();
    }

    ~RotorVisualizationTest() {
        cleanup();
    }

private:
    RotorTestConfig m_config;
    std::unique_ptr<visualization::DronePhysicsRenderer> m_renderer;
    std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfiguration;
    std::unique_ptr<btCollisionDispatcher> m_dispatcher;
    std::unique_ptr<btBroadphaseInterface> m_broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_dynamicsWorld;
    btRigidBody* m_rotorBody{nullptr};
    btRigidBody* m_groundBody{nullptr};
    util::vector<rotor::drone_component> m_rotorComponent;
    std::vector<glm::vec3> m_trajectoryPoints;  // Store points for trajectory visualization
    float m_timeSinceLastLog{0.0f};

    bool shouldContinue(float testTime) {
        if (m_config.visual_mode) {
            return !m_renderer->shouldClose();
        }
        return testTime < m_config.test_duration;
    }

    void logConfiguration() {
        std::cout << std::fixed << std::setprecision(3)
                  << "Rotor Physics Test Configuration:\n"
                  << "- Mode: " << (m_config.visual_mode ? "Visual" : "Console") << "\n"
                  << "- Simulation Speed: " << m_config.simulation_speed << "x\n"
                  << "- Target RPM: " << m_config.target_rpm << "\n"
                  << "- Test Duration: " << m_config.test_duration << " seconds\n"
                  << std::endl;
    }

    void runGroundEffectTest() {
        float height = 3.0f;  // Start at 3 rotor diameters
        const float min_height = 0.1f;
        float descent_rate = 0.1f;

        m_rotorComponent[0].set_rpm(m_config.target_rpm);

        while (height > min_height && shouldContinue(0.0f)) {
            // Update height
            btTransform trans;
            trans.setIdentity();
            trans.setOrigin(btVector3(0.0f, height, 0.0f));
            m_rotorBody->getMotionState()->setWorldTransform(trans);
            m_rotorBody->setWorldTransform(trans);

            // Run simulation step
            updatePhysics(1.0f/60.0f);

            // Log data
            float thrust = m_rotorComponent[0].get_thrust();
            float power = m_rotorComponent[0].get_power_consumption();
            std::cout << "Height: " << height << "m, "
                      << "Thrust: " << thrust << "N, "
                      << "Power: " << power << "W, "
                      << "Efficiency: " << thrust/power << "N/W" << std::endl;

            if (m_config.visual_mode && m_renderer) {
                renderFrame();
                std::this_thread::sleep_for(std::chrono::milliseconds(16));
            }

            height -= descent_rate * (1.0f/60.0f);
        }
    }

    void logState(float testTime) {
        btTransform trans;
        m_rotorBody->getMotionState()->getWorldTransform(trans);
        btVector3 velocity = m_rotorBody->getLinearVelocity();

        std::cout << "Time: " << testTime << "s\n"
                  << "Position: (" << trans.getOrigin().getX() << ", "
                  << trans.getOrigin().getY() << ", "
                  << trans.getOrigin().getZ() << ") m\n"
                  << "Velocity: (" << velocity.getX() << ", "
                  << velocity.getY() << ", "
                  << velocity.getZ() << ") m/s\n"
                  << "Thrust: " << m_rotorComponent[0].get_thrust() << " N\n"
                  << "Power: " << m_rotorComponent[0].get_power_consumption() << " W\n"
                  << std::endl;
    }

    static glm::mat4 bulletToGlm(const btTransform& t) {
        btScalar m[16];
        t.getOpenGLMatrix(m);
        return glm::make_mat4(m);
    }

    void updatePhysics(float deltaTime) {
        m_rotorComponent[0].calculate_forces(deltaTime);
        m_dynamicsWorld->stepSimulation(deltaTime, 10);
    }

    void renderFrame() {
        if (!m_renderer) return;

        m_renderer->clear();

        btTransform trans;
        m_rotorBody->getMotionState()->getWorldTransform(trans);

        // Set camera to follow rotor
        m_renderer->setCameraTarget(glm::vec3(
            trans.getOrigin().getX(),
            trans.getOrigin().getY(),
            trans.getOrigin().getZ()
        ));

        // Convert bullet transform to GLM properly
        glm::mat4 transform(1.0f);
        // Get rotation
        const btMatrix3x3& basis = trans.getBasis();
        for(int row = 0; row < 3; row++) {
            for(int col = 0; col < 3; col++) {
                transform[col][row] = basis[row][col];
            }
        }
        // Get translation
        const btVector3& origin = trans.getOrigin();
        transform[3][0] = origin.getX();
        transform[3][1] = origin.getY();
        transform[3][2] = origin.getZ();

        // Add rotor visualization with proper scale
        m_renderer->addObject(transform, glm::vec3(0.7f, 0.2f, 0.2f), glm::vec3(0.4f, 0.05f, 0.4f));

        // Add a small center indicator for the rotation axis
        glm::mat4 centerTransform = transform;
        // Keep the translation but scale around the center
        centerTransform = glm::translate(glm::mat4(1.0f), glm::vec3(transform[3])) *
                         glm::scale(glm::mat4(1.0f), glm::vec3(0.1f));
        m_renderer->addObject(centerTransform, glm::vec3(1.0f, 1.0f, 1.0f));

        // Add trajectory visualization
        if (!m_trajectoryPoints.empty()) {
            int i = 0;
            for (const auto& point : m_trajectoryPoints) {
                i++;
                if ((i % 20) != 0) continue;
                glm::mat4 pointTransform = glm::translate(glm::mat4(1.0f), point);
                pointTransform = glm::scale(pointTransform, glm::vec3(0.05f));
                m_renderer->addObject(pointTransform, glm::vec3(0.2f, 0.7f, 0.2f));
            }
        }

        m_renderer->render();

        // Store trajectory points
        m_trajectoryPoints.push_back(glm::vec3(transform[3]));
    }

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

        m_dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));
        createGround();
    }

    void createGround() {
        btCollisionShape* groundShape = new btBoxShape(btVector3(50.0f, 1.0f, 50.0f));
        btTransform groundTransform;
        groundTransform.setIdentity();
        groundTransform.setOrigin(btVector3(0.0f, -1.0f, 0.0f));

        btScalar mass(0.0f);
        btVector3 localInertia(0, 0, 0);

        btDefaultMotionState* groundMotionState = new btDefaultMotionState(groundTransform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, groundMotionState, groundShape, localInertia);
        m_groundBody = new btRigidBody(rbInfo);
        m_dynamicsWorld->addRigidBody(m_groundBody);
    }

    void setupRotor() {
        fuselage::init_info fuselageInfo;
        rotor::init_info rotorInfo;

        // Configure rotor parameters
        rotorInfo.bladeRadius = 0.2;     // 12.7 cm propeller
        rotorInfo.bladePitch = 0.2f;     // ~11.5 degrees
        rotorInfo.bladeCount = 3;        // Three-blade propeller
        rotorInfo.mass = 0.5f;           // 500g
        rotorInfo.rotorNormal = btVector3(0, 1, 0);
        // Set initial height to 1.0 unit above ground
        rotorInfo.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        createRotorBody(rotorInfo);
        rotorInfo.rigidBody = m_rotorBody;

        drone_entity::entity_info info{};
        info.fuselage = &fuselageInfo;
        info.rotors = { &rotorInfo };

        auto entity = drone_entity::create(info);
        assert(entity.is_valid());

        m_rotorComponent = entity.rotor();
        assert(m_rotorComponent[0].is_valid());
        m_rotorComponent[0].initialize();
    }

    void createRotorBody(const rotor::init_info& rotorInfo) {
        btCollisionShape* rotorShape = new btBoxShape(btVector3(
            rotorInfo.bladeRadius,
            0.02f,
            rotorInfo.bladeRadius
        ));

        btTransform startTransform;
        startTransform.setIdentity();
        startTransform.setOrigin(util::glm_to_bt_vector3(glm::vec3(rotorInfo.transform[3])));

        btVector3 localInertia(0, 0, 0);
        rotorShape->calculateLocalInertia(rotorInfo.mass, localInertia);

        btDefaultMotionState* motionState = new btDefaultMotionState(startTransform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(
            rotorInfo.mass,
            motionState,
            rotorShape,
            localInertia
        );

        m_rotorBody = new btRigidBody(rbInfo);
        m_rotorBody->setDamping(0.1f, 0.3f);
        m_rotorBody->setAngularFactor(btVector3(0, 1, 0));
        m_dynamicsWorld->addRigidBody(m_rotorBody);
    }

    void cleanup() {
        if (m_rotorBody) {
            m_dynamicsWorld->removeRigidBody(m_rotorBody);
            delete m_rotorBody->getMotionState();
            delete m_rotorBody->getCollisionShape();
            delete m_rotorBody;
        }
        if (m_groundBody) {
            m_dynamicsWorld->removeRigidBody(m_groundBody);
            delete m_groundBody->getMotionState();
            delete m_groundBody->getCollisionShape();
            delete m_groundBody;
        }
    }
};

} // namespace lark::physics