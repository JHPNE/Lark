// Tests/PhysicsTests/RotorPhysicsTest.h
#pragma once
#include "BulletCollision/BroadphaseCollision/btDbvtBroadphase.h"
#include "BulletCollision/CollisionDispatch/btCollisionDispatcher.h"
#include "BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h"
#include "BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h"
#include "BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h"
#include "DroneExtension/Components/Physics/RotorPhysics.h"
#include <gtest/gtest.h>
#include <memory>

namespace lark::tests {

class RotorPhysicsTest : public ::testing::Test {
protected:
    void SetUp() override;
    void TearDown() override;
    void setupDefaultRotor();
    models::AtmosphericConditions getStandardConditions();

protected:
    std::unique_ptr<btDefaultCollisionConfiguration> m_collisionConfiguration;
    std::unique_ptr<btCollisionDispatcher> m_dispatcher;
    std::unique_ptr<btBroadphaseInterface> m_broadphase;
    std::unique_ptr<btSequentialImpulseConstraintSolver> m_solver;
    std::unique_ptr<btDiscreteDynamicsWorld> m_dynamicsWorld;

    drone_data::RotorBody m_rotorData;
    btRigidBody* m_rotorBody{nullptr};
};

}