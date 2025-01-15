#pragma once
#include "DroneExtension/Components/Rotor.h"
#include "DronePhysicsRenderer.h"
#include <btBulletDynamicsCommon.h>
#include <chrono>
#include <memory>
#include <thread>
#include <iomanip>

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
        float testTime = 0.0f;

        // Test parameters
        const float target_rpm = 7000.0f;
        m_rotorComponent.set_rpm(target_rpm);

        std::cout << std::fixed << std::setprecision(3);
        std::cout << "Starting rotor physics test...\n"
                  << "Configuration:\n"
                  << "- Target RPM: " << target_rpm << "\n"
                  << "- Blade Count: " << 2 << "\n"
                  << "- Blade Radius: " << 0.127f << "m\n"
                  << std::endl;

        while (!m_renderer.shouldClose()) {
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
            lastTime = currentTime;
            testTime += deltaTime;

            // Update physics
            updatePhysics(deltaTime);

            // Log state every second
            m_timeSinceLastLog += deltaTime;
            if (m_timeSinceLastLog >= 1.0f) {
                logState();
                m_timeSinceLastLog = 0.0f;
            }

            // Render current state
            renderFrame();

            // Maintain consistent frame rate
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
    float m_timeSinceLastLog{0.0f};

    void logState() {
        btTransform trans;
        m_rotorBody->getMotionState()->getWorldTransform(trans);
        btVector3 velocity = m_rotorBody->getLinearVelocity();

        std::cout << "Physics State:\n"
                  << "Position: ("
                  << trans.getOrigin().getX() << ", "
                  << trans.getOrigin().getY() << ", "
                  << trans.getOrigin().getZ() << ") m\n"
                  << "Velocity: ("
                  << velocity.getX() << ", "
                  << velocity.getY() << ", "
                  << velocity.getZ() << ") m/s\n"
                  << "Thrust: " << m_rotorComponent.get_thrust() << " N\n"
                  << "Power: " << m_rotorComponent.get_power_consumption() << " W\n"
                  << "Height: " << trans.getOrigin().getY() << " m\n"
                  << std::endl;
    }

    static glm::mat4 bulletToGlm(const btTransform& t) {
        btScalar m[16];
        t.getOpenGLMatrix(m);
        return glm::make_mat4(m);
    }

    void updatePhysics(float deltaTime) {
        m_rotorComponent.calculate_forces(deltaTime);
        m_dynamicsWorld->stepSimulation(deltaTime, 10);
    }

    void renderFrame() {
        btTransform trans;
        m_rotorBody->getMotionState()->getWorldTransform(trans);

        m_renderer.setObjectTransform(bulletToGlm(trans));
        m_renderer.setCameraTarget(glm::vec3(
            trans.getOrigin().getX(),
            trans.getOrigin().getY(),
            trans.getOrigin().getZ()
        ));
        m_renderer.render();
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
        rotorInfo.bladeRadius = 0.127f;     // 5-inch propeller
        rotorInfo.bladePitch = 0.2f;        // ~11.5 degrees
        rotorInfo.bladeCount = 2;           // Dual-blade propeller
        rotorInfo.liftCoefficient = 0.12f;  // Typical value
        rotorInfo.mass = 0.25f;             // 250g
        rotorInfo.rotorNormal = btVector3(0, 1, 0);
        rotorInfo.position = btVector3(0, 0.5f, 0);

        createRotorBody(rotorInfo);
        rotorInfo.rigidBody = m_rotorBody;

        drone_entity::entity_info info{};
        info.fuselage = &fuselageInfo;
        info.rotor = &rotorInfo;

        auto entity = drone_entity::create(info);
        assert(entity.is_valid());

        m_rotorComponent = entity.rotor();
        assert(m_rotorComponent.is_valid());
        m_rotorComponent.initialize();
    }

    void createRotorBody(const rotor::init_info& rotorInfo) {
        btCollisionShape* rotorShape = new btBoxShape(btVector3(
            rotorInfo.bladeRadius,
            0.02f,
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