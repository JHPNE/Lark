#pragma once
#include "DroneExtension/Components/Rotor.h"
#include "DronePhysicsRenderer.h"
#include <btBulletDynamicsCommon.h>
#include <chrono>
#include <memory>
#include <thread>

namespace lark::physics {

class RotorVisualizationTest {
public:
    RotorVisualizationTest() : m_renderer(1280, 720) {
        initializePhysics();
        setupRotor();
    }

    void run() {
        const float timeStep = 1.0f / 60.0f;
        auto lastTime = std::chrono::high_resolution_clock::now();
        const float target_rpm = 5000.0f;
        float predicted_height = m_rotorComponent.estimate_equilibrium_height(target_rpm);
        float max_theoretical = m_rotorComponent.get_max_theoretical_height(target_rpm);

        std::cout << "Starting rotor physics test..." << std::endl;
        std::cout << "Initial RPM: 5000.0" << std::endl;
        std::cout << "Predicted equilibrium height: " << predicted_height << " meters" << std::endl;
        std::cout << "Maximum theoretical height: " << max_theoretical << " meters" << std::endl;

        while (!m_renderer.shouldClose()) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
            lastTime = currentTime;

            // Update physics
            m_rotorComponent.calculate_forces(deltaTime);
            m_dynamicsWorld->stepSimulation(deltaTime, 10);

            // Log state periodically
            static float timeSinceLastLog = 0.0f;
            timeSinceLastLog += deltaTime;

            if (timeSinceLastLog >= 1.0f) {
                btTransform trans;
                m_rotorBody->getMotionState()->getWorldTransform(trans);

                btVector3 velocity = m_rotorBody->getLinearVelocity();
                btVector3 angularVel = m_rotorBody->getAngularVelocity();

                std::cout << "Current State:\n"
                          << "Position = (" << trans.getOrigin().getX() << ", "
                          << trans.getOrigin().getY() << ", "
                          << trans.getOrigin().getZ() << ")\n"
                          << "Linear Velocity = (" << velocity.getX() << ", "
                          << velocity.getY() << ", " << velocity.getZ() << ")\n"
                          << "Angular Velocity = (" << angularVel.getX() << ", "
                          << angularVel.getY() << ", " << angularVel.getZ() << ")\n"
                          << "Thrust = " << m_rotorComponent.get_thrust() << " N\n"
                          << "Power = " << m_rotorComponent.get_power_consumption() << " W\n"
                          << std::endl;

                timeSinceLastLog = 0.0f;
            }

            // Render
            btTransform trans;
            m_rotorBody->getMotionState()->getWorldTransform(trans);
            m_renderer.setObjectTransform(bulletToGlm(trans));
            m_renderer.setCameraTarget(glm::vec3(
                trans.getOrigin().getX(),
                trans.getOrigin().getY(),
                trans.getOrigin().getZ()
            ));
            m_renderer.render();

            // Cap frame rate
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    ~RotorVisualizationTest() {
        cleanup();
    }

private:
    visualization::DronePhysicsRenderer m_renderer;
    std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfiguration;
    std::unique_ptr<btCollisionDispatcher> m_dispatcher;
    std::unique_ptr<btBroadphaseInterface> m_broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_dynamicsWorld;
    btRigidBody* m_rotorBody{nullptr};
    btRigidBody* m_groundBody{nullptr};
    rotor::drone_component m_rotorComponent;

    static glm::mat4 bulletToGlm(const btTransform& t) {
        btScalar m[16];
        t.getOpenGLMatrix(m);
        return glm::make_mat4(m);
    }

    void initializePhysics() {
        // Create physics world
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

        // Create ground
        btBoxShape* groundShape = new btBoxShape(btVector3(50.0f, 1.0f, 50.0f));
        btTransform groundTransform;
        groundTransform.setIdentity();
        groundTransform.setOrigin(btVector3(0.0f, -1.0f, 0.0f));

        btScalar mass(0.0f);  // Mass = 0 for static ground
        btVector3 localInertia(0, 0, 0);

        btDefaultMotionState* groundMotionState = new btDefaultMotionState(groundTransform);
        btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, groundMotionState, groundShape, localInertia);
        m_groundBody = new btRigidBody(rbInfo);
        m_dynamicsWorld->addRigidBody(m_groundBody);
    }

    void setupRotor() {
        // Create rotor entity
        fuselage::init_info fuselageInfo;
        rotor::init_info rotorInfo;

        // Set up realistic rotor parameters for a small drone propeller
        rotorInfo.bladeRadius = 0.127f;     // 5-inch propeller (0.127m)
        rotorInfo.bladePitch = 0.2f;        // ~11.5 degrees in radians
        rotorInfo.bladeCount = 2;           // Standard dual-blade propeller
        rotorInfo.airDensity = 1.225f;      // Sea level air density
        rotorInfo.discArea = glm::pi<float>() * rotorInfo.bladeRadius * rotorInfo.bladeRadius;
        rotorInfo.liftCoefficient = 0.12f;  // Typical for drone propellers
        rotorInfo.mass = 0.05f;             // 50g total mass
        rotorInfo.rotorNormal = btVector3(0, 1, 0);  // Upward thrust
        rotorInfo.position = btVector3(0, 0.5f, 0);  // Starting position

        // Create a simple box shape for the rotor
        btBoxShape* rotorShape = new btBoxShape(btVector3(
            rotorInfo.bladeRadius,
            0.02f,                          // Thin profile
            rotorInfo.bladeRadius
        ));

        btTransform startTransform;
        startTransform.setIdentity();
        startTransform.setOrigin(rotorInfo.position);

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
        m_rotorBody->setAngularFactor(btVector3(0, 1, 0));  // Only allow rotation around Y axis

        m_dynamicsWorld->addRigidBody(m_rotorBody);
        rotorInfo.rigidBody = m_rotorBody;

        // Create entity info and set components
        drone_entity::entity_info info{};
        info.fuselage = &fuselageInfo;
        info.rotor = &rotorInfo;

        // Create drone entity and initialize
        auto entity = drone_entity::create(info);
        assert(entity.is_valid());

        m_rotorComponent = entity.rotor();
        assert(m_rotorComponent.is_valid());
        m_rotorComponent.initialize();
    }
    void cleanup() {
        if (m_rotorBody) {
            m_dynamicsWorld->removeRigidBody(m_rotorBody);
            delete m_rotorBody->getMotionState();
            delete m_rotorBody;
        }
        if (m_groundBody) {
            m_dynamicsWorld->removeRigidBody(m_groundBody);
            delete m_groundBody->getMotionState();
            delete m_groundBody;
        }
    }
};

} // namespace lark::physics // namespace lark::physics