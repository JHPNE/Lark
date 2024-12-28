#include "BodySystem.h"
#include <algorithm>
#include <cmath>

namespace drosim::physics::cpu {
    namespace {
        bool isValidId(uint32_t id) {
            return id != UINT32_MAX;
        }

        void ValidateBody(RigidBody& body) {
            // Only check for NaN values, no artificial limits
            bool hasNaN = std::isnan(body.motion.position.x) ||
                         std::isnan(body.motion.position.y) ||
                         std::isnan(body.motion.position.z) ||
                         std::isnan(body.motion.velocity.x) ||
                         std::isnan(body.motion.velocity.y) ||
                         std::isnan(body.motion.velocity.z) ||
                         std::isnan(body.motion.angularVelocity.x) ||
                         std::isnan(body.motion.angularVelocity.y) ||
                         std::isnan(body.motion.angularVelocity.z);

            if (hasNaN) {
                // Reset only if we detect invalid state
                body.motion.position = glm::vec3(0.0f);
                body.motion.velocity = glm::vec3(0.0f);
                body.motion.angularVelocity = glm::vec3(0.0f);
                body.motion.orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
            }

            // Always normalize quaternion to prevent drift
            body.motion.orientation = glm::normalize(body.motion.orientation);
        }

        void UpdateInertialProperties(RigidBody& body) {
            // Update inverse inertia tensor in world space
            glm::mat3 R = glm::mat3_cast(body.motion.orientation);
            body.inertia.globalInvInertia = R * body.inertia.invLocalInertia * glm::transpose(R);
        }
    }

    uint32_t CreateBody(PhysicsWorld& world, const glm::vec3& pos, float mass) {
        uint32_t idx = static_cast<uint32_t>(world.bodyPool.Allocate());
        assert(isValidId(idx));

        auto& body = world.bodyPool[idx];

        // Motion state
        body.motion.position = pos;
        body.motion.velocity = glm::vec3(0.0f);
        body.motion.angularVelocity = glm::vec3(0.0f);
        body.motion.orientation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);

        // Inertial properties
        body.inertia.mass = mass;
        body.inertia.invMass = (mass > 0.0f) ? 1.0f / mass : 0.0f;

        // Default to unit inertia tensor (should be updated when adding colliders)
        body.inertia.localInertia = glm::mat3(1.0f);
        body.inertia.invLocalInertia = glm::mat3(1.0f);
        UpdateInertialProperties(body);

        // Clear forces
        body.forces.force = glm::vec3(0.0f);
        body.forces.torque = glm::vec3(0.0f);

        // Material properties
        body.material.friction = 0.7f;
        body.material.restitution = 0.2f;

        // Flags
        body.flags.active = (mass > 0.0f);
        body.flags.isStatic = (mass == 0.0f);

        return idx;
    }

    glm::vec3 GetBodyPosition(PhysicsWorld& world, uint32_t id) {
        return world.bodyPool[id].motion.position;
    }

    void IntegrateForces(PhysicsWorld& world, float dt) {
        // Use fixed sub-stepping for better stability
        const float fixedDt = 1.0f / 240.0f;  // 240 Hz internal physics rate
        int numSubSteps = static_cast<int>(dt / fixedDt) + 1;
        float subDt = dt / static_cast<float>(numSubSteps);

        for (int step = 0; step < numSubSteps; ++step) {
            for (size_t i = 0; i < world.bodyPool.Size(); ++i) {
                auto& body = world.bodyPool[i];
                if (!body.flags.active || body.flags.isStatic) continue;

                // Semi-implicit Euler integration
                glm::vec3 totalForce = body.forces.force + world.gravity * body.inertia.mass;

                // Update linear velocity (v = v0 + a*dt)
                body.motion.velocity += totalForce * body.inertia.invMass * subDt;

                // Update angular velocity (ω = ω0 + α*dt)
                body.motion.angularVelocity += body.inertia.globalInvInertia * body.forces.torque * subDt;

                // Validate state
                ValidateBody(body);
            }
        }
    }

    void IntegrateVelocities(PhysicsWorld& world, float dt) {
        const float fixedDt = 1.0f / 240.0f;
        int numSubSteps = static_cast<int>(dt / fixedDt) + 1;
        float subDt = dt / static_cast<float>(numSubSteps);

        for (int step = 0; step < numSubSteps; ++step) {
            for (size_t i = 0; i < world.bodyPool.Size(); ++i) {
                auto& body = world.bodyPool[i];
                if (!body.flags.active || body.flags.isStatic) continue;

                // Store previous state
                glm::vec3 oldPos = body.motion.position;
                glm::quat oldRot = body.motion.orientation;

                // Update position (x = x0 + v*dt)
                body.motion.position += body.motion.velocity * subDt;

                // Update orientation
                glm::vec3 omega = body.motion.angularVelocity;
                float omegaMag = glm::length(omega);

                if (omegaMag > 1e-6f) {
                    glm::vec3 axis = omega / omegaMag;
                    float halfAngle = omegaMag * subDt * 0.5f;
                    float sinHalf = std::sin(halfAngle);

                    glm::quat deltaRot(std::cos(halfAngle),
                                     axis.x * sinHalf,
                                     axis.y * sinHalf,
                                     axis.z * sinHalf);

                    body.motion.orientation = glm::normalize(deltaRot * body.motion.orientation);
                }

                // Check for invalid state
                bool stateInvalid = std::isnan(body.motion.position.x) ||
                                  std::isnan(body.motion.position.y) ||
                                  std::isnan(body.motion.position.z) ||
                                  std::isnan(body.motion.orientation.x) ||
                                  std::isnan(body.motion.orientation.y) ||
                                  std::isnan(body.motion.orientation.z) ||
                                  std::isnan(body.motion.orientation.w);

                if (stateInvalid) {
                    // Revert to previous valid state
                    body.motion.position = oldPos;
                    body.motion.orientation = oldRot;
                    body.motion.velocity = glm::vec3(0.0f);
                    body.motion.angularVelocity = glm::vec3(0.0f);
                }

                // Update inertia tensor
                UpdateInertialProperties(body);
            }
        }
    }

    void UpdateSleeping(PhysicsWorld& world) {
        const float SLEEP_LINEAR_THRESHOLD = 0.01f;  // m/s
        const float SLEEP_ANGULAR_THRESHOLD = 0.01f; // rad/s
        const float SLEEP_TIME = 0.5f;              // seconds to sleep

        for (size_t i = 0; i < world.bodyPool.Size(); ++i) {
            auto& body = world.bodyPool[i];
            if (body.flags.isStatic) continue;

            float linSpeed = glm::length(body.motion.velocity);
            float angSpeed = glm::length(body.motion.angularVelocity);

            if (linSpeed < SLEEP_LINEAR_THRESHOLD &&
                angSpeed < SLEEP_ANGULAR_THRESHOLD) {
                // Could add sleep timer here for more sophisticated sleeping
                body.flags.active = false;
                body.motion.velocity = glm::vec3(0.0f);
                body.motion.angularVelocity = glm::vec3(0.0f);
            } else {
                body.flags.active = true;
            }
        }
    }

    void ClearForces(PhysicsWorld& world) {
        for (size_t i = 0; i < world.bodyPool.Size(); ++i) {
            world.bodyPool[i].forces.force = glm::vec3(0.0f);
            world.bodyPool[i].forces.torque = glm::vec3(0.0f);
        }
    }
}