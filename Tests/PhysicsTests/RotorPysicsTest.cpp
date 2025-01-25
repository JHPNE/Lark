#include "RotorPhysicsTest.h"

namespace lark::tests {

void RotorPhysicsTest::SetUp() {
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

  setupDefaultRotor();
}

void RotorPhysicsTest::TearDown() {
  if (m_rotorBody) {
    m_dynamicsWorld->removeRigidBody(m_rotorBody);
    delete m_rotorBody->getMotionState();
    delete m_rotorBody->getCollisionShape();
    delete m_rotorBody;
  }
}

void RotorPhysicsTest::setupDefaultRotor() {
    // Create a standard test rotor configuration
    m_rotorData.bladeRadius = 0.2f;  // 20cm radius
    m_rotorData.bladePitch = 0.2f;   // ~11.5 degrees
    m_rotorData.bladeCount = 2;
    m_rotorData.mass = 0.1f;         // 100g
    m_rotorData.discArea = models::PI * m_rotorData.bladeRadius * m_rotorData.bladeRadius;
    m_rotorData.currentRPM = 0.0f;
    m_rotorData.rotorNormal = btVector3(0, 1, 0);

    // Create rigid body
    btCollisionShape* shape = new btCylinderShape(
        btVector3(m_rotorData.bladeRadius, 0.02f, m_rotorData.bladeRadius));

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(0, 1, 0));  // 1m above ground

    btVector3 localInertia(0, 0, 0);
    shape->calculateLocalInertia(m_rotorData.mass, localInertia);

    btDefaultMotionState* motionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(
        m_rotorData.mass, motionState, shape, localInertia);

    m_rotorBody = new btRigidBody(rbInfo);
    m_rotorBody->setDamping(0.1f, 0.1f);
    m_dynamicsWorld->addRigidBody(m_rotorBody);

    m_rotorData.rigidBody = m_rotorBody;
    m_rotorData.dynamics_world = m_dynamicsWorld.get();

    // Initialize physics properties
    rotor::physics::initialize_blade_properties(&m_rotorData);
    rotor::physics::initialize_motor_parameters(&m_rotorData);
}

models::AtmosphericConditions RotorPhysicsTest::getStandardConditions() {
  return models::calculate_atmospheric_conditions(0.0f, 0.0f);
}

// Test cases
TEST_F(RotorPhysicsTest, HoverThrustTest) {
  const float test_rpm = 5000.0f;
  m_rotorData.currentRPM = test_rpm;

  auto conditions = getStandardConditions();
  float thrust = rotor::physics::calculate_thrust(&m_rotorData, conditions);

  EXPECT_GT(thrust, 0.0f);
  EXPECT_LT(thrust, 10.0f);

  m_rotorData.currentRPM = test_rpm * 2.0f;
  float thrust_2x = rotor::physics::calculate_thrust(&m_rotorData, conditions);
  EXPECT_NEAR(thrust_2x / thrust, 4.0f, 0.1f);
}

// Test power calculation
TEST_F(RotorPhysicsTest, PowerCalculationTest) {
    const float test_rpm = 5000.0f;
    m_rotorData.currentRPM = test_rpm;

    auto conditions = getStandardConditions();
    float thrust = rotor::physics::calculate_thrust(&m_rotorData, conditions);
    float power = rotor::physics::calculate_power(&m_rotorData, thrust, conditions);

    // Basic sanity checks
    EXPECT_GT(power, 0.0f);

    // Verify power scales with RPM cubed
    m_rotorData.currentRPM = test_rpm * 2.0f;
    float thrust_2x = rotor::physics::calculate_thrust(&m_rotorData, conditions);
    float power_2x = rotor::physics::calculate_power(&m_rotorData, thrust_2x, conditions);
    EXPECT_NEAR(power_2x / power, 8.0f, 0.2f);  // Should be ~8x at 2x RPM
}

// Test ground effect
TEST_F(RotorPhysicsTest, GroundEffectTest) {
    m_rotorData.currentRPM = 5000.0f;
    auto conditions = getStandardConditions();

    // Get baseline thrust at height
    btTransform transform = m_rotorBody->getWorldTransform();
    transform.setOrigin(btVector3(0, 2.0f * m_rotorData.bladeRadius * 2, 0));
    m_rotorBody->setWorldTransform(transform);

    float baseline_thrust = rotor::physics::calculate_thrust(&m_rotorData, conditions);

    // Test at 0.5 rotor diameters
    transform.setOrigin(btVector3(0, m_rotorData.bladeRadius, 0));
    m_rotorBody->setWorldTransform(transform);
    float ground_thrust = rotor::physics::calculate_thrust(&m_rotorData, conditions);

    // Should see increased thrust near ground
    EXPECT_GT(ground_thrust, baseline_thrust);
    EXPECT_LT(ground_thrust / baseline_thrust, 1.4f);  // Maximum theoretical increase
}

// Test blade flapping dynamics
TEST_F(RotorPhysicsTest, BladeFlappingTest) {
    m_rotorData.currentRPM = 5000.0f;
    auto conditions = getStandardConditions();

    // Test forward flight condition
    m_rotorBody->setLinearVelocity(btVector3(5.0f, 0, 0));  // 5 m/s forward

    rotor::physics::update_blade_state(&m_rotorData, 5.0f, conditions, 0.016f);

    // Check reasonable flapping angles
    EXPECT_GT(m_rotorData.blade_state.flapping_angle, 0.0f);
    EXPECT_LT(m_rotorData.blade_state.flapping_angle, 0.2f);  // Should be less than ~11.5 degrees

    // Check coning angle
    EXPECT_GT(m_rotorData.blade_state.coning_angle, 0.0f);
    EXPECT_LT(m_rotorData.blade_state.coning_angle, 0.15f);
}

// Test turbulence response
TEST_F(RotorPhysicsTest, TurbulenceTest) {
    // Test parameters
    const float test_altitude = 100.0f;  // 100m
    const float test_airspeed = 15.0f;  // 15 m/s (reasonable airspeed)
    const float test_time1 = 1.0f;
    const float test_time2 = 2.0f;

    // Atmospheric conditions
    auto conditions = getStandardConditions();

    // Calculate turbulence states at different time steps
    auto turb_state1 = models::calculate_turbulence(test_altitude, test_airspeed, conditions, test_time1);
    auto turb_state2 = models::calculate_turbulence(test_altitude, test_airspeed, conditions, test_time2);

    // Validate turbulence velocities
    EXPECT_GT(turb_state1.velocity.length(), 0.0f);  // Non-zero turbulence
    EXPECT_LT(turb_state1.velocity.length(), 10.0f);  // Light to moderate turbulence range
    EXPECT_GT(turb_state1.angular_velocity.length(), 0.0f);
    EXPECT_LT(turb_state1.angular_velocity.length(), 2.0f);  // Reasonable angular turbulence

    // Validate temporal variation (turbulence should change over time)
    float velocity_change = (turb_state2.velocity - turb_state1.velocity).length();
    EXPECT_GT(velocity_change, 0.05f);

    // Ensure turbulence magnitude increases with altitude
    auto turb_state_low = models::calculate_turbulence(10.0f, test_airspeed, conditions, test_time1);
    EXPECT_GT(turb_state1.velocity.length(), turb_state_low.velocity.length() * 0.9f);
}


// Test motor dynamics
TEST_F(RotorPhysicsTest, MotorDynamicsTest) {
    const float test_rpm = 5000.0f;
    m_rotorData.currentRPM = test_rpm;
    auto conditions = getStandardConditions();

    rotor::physics::update_motor_state(&m_rotorData, conditions, 0.016f);

    // Check motor state
    EXPECT_GT(m_rotorData.motor_state.power_consumption, 0.0f);
    EXPECT_LT(m_rotorData.motor_state.winding_temperature, 150.0f); // Reasonable temp
    EXPECT_GT(m_rotorData.motor_state.efficiency, 0.5f);  // Should be reasonably efficient
    EXPECT_LT(m_rotorData.motor_state.efficiency, 1.0f);  // But not over 100%
}

// Test full physics update
TEST_F(RotorPhysicsTest, FullPhysicsUpdateTest) {
    const float test_rpm = 5000.0f;
    m_rotorData.currentRPM = test_rpm;

    // Record initial state
    btTransform initial_transform = m_rotorBody->getWorldTransform();
    btVector3 initial_vel = m_rotorBody->getLinearVelocity();

    // Run multiple physics steps
    for (int i = 0; i < 60; i++) {  // Simulate 1 second
        auto conditions = getStandardConditions();
        float thrust = rotor::physics::calculate_thrust(&m_rotorData, conditions);

        rotor::physics::update_blade_state(&m_rotorData,
            m_rotorBody->getLinearVelocity().length(), conditions, 0.016f);
        rotor::physics::update_motor_state(&m_rotorData, conditions, 0.016f);
        rotor::physics::apply_turbulence(&m_rotorData, conditions, 0.016f);

        m_dynamicsWorld->stepSimulation(0.016f);
    }

    // Check final state
    btTransform final_transform = m_rotorBody->getWorldTransform();
    btVector3 final_vel = m_rotorBody->getLinearVelocity();

    // Should have lifted due to thrust
    EXPECT_GT(final_transform.getOrigin().y(), initial_transform.getOrigin().y());

    // Velocity should be reasonable
    EXPECT_LT(final_vel.length(), 10.0f);
}
} // namespace lark::tests