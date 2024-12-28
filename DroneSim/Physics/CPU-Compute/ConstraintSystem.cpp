#include "ConstraintSystem.h"
#include "BodySystem.h"
#include <cmath>

namespace drosim::physics::cpu {
    void CreateDistanceConstraint(PhysicsWorld& world,
                                  uint32_t bodyA, uint32_t bodyB,
                                  const glm::vec3& localAnchorA,
                                  const glm::vec3& localAnchorB,
                                  float restLength)
    {
        ConstraintInfo c;
        c.bodyA = bodyA;
        c.bodyB = bodyB;
        c.localAnchorA = localAnchorA;
        c.localAnchorB = localAnchorB;
        c.restLength = restLength;
        world.constraints.push_back(c);
    }

    void SolveConstraints(PhysicsWorld& world, float dt, int iterations)
    {
        // Contacts (from NarrowPhase) + user constraints
        for (int iter = 0; iter < iterations; iter++) {
            // 1) Solve contact constraints
            for (auto& cp : world.contacts) {
                auto& bodyA = world.bodyPool[cp.bodyAIndex];
                auto& bodyB = world.bodyPool[cp.bodyBIndex];
                if (!bodyA.flags.active && !bodyB.flags.active) continue;
                // ... normal impulse, friction, etc.
            }

            // 2) Solve user constraints (distance, hinge, etc.)
            for (auto& c : world.constraints) {
                auto& bodyA = world.bodyPool[c.bodyA];
                auto& bodyB = world.bodyPool[c.bodyB];
                if (!bodyA.flags.active && !bodyB.flags.active) continue;

                glm::vec3 worldAnchorA = bodyA.motion.position
                    + bodyA.motion.orientation * c.localAnchorA;
                glm::vec3 worldAnchorB = bodyB.motion.position
                    + bodyB.motion.orientation * c.localAnchorB;
                glm::vec3 diff = worldAnchorB - worldAnchorA;
                float dist = glm::length(diff);
                if (dist < 1e-6f) continue;
                float error = dist - c.restLength;
                glm::vec3 dir = diff / dist;

                float invMassA = bodyA.inertia.invMass;
                float invMassB = bodyB.inertia.invMass;
                glm::vec3 rA = worldAnchorA - bodyA.motion.position;
                glm::vec3 rB = worldAnchorB - bodyB.motion.position;

                glm::vec3 crossA = bodyA.inertia.globalInvInertia * glm::cross(rA, dir);
                glm::vec3 crossB = bodyB.inertia.globalInvInertia * glm::cross(rB, dir);

                float denom = invMassA + invMassB
                    + glm::dot(dir, glm::cross(crossA, rA))
                    + glm::dot(dir, glm::cross(crossB, rB));

                if (denom < 1e-6f) continue;
                float lambda = -error / denom;
                glm::vec3 impulse = dir * lambda;

                // apply
                if (invMassA > 0.f && bodyA.flags.active) {
                    bodyA.motion.velocity -= impulse * invMassA;
                    bodyA.motion.angularVelocity -= bodyA.inertia.globalInvInertia
                        * glm::cross(rA, impulse);
                }
                if (invMassB > 0.f && bodyB.flags.active) {
                    bodyB.motion.velocity += impulse * invMassB;
                    bodyB.motion.angularVelocity += bodyB.inertia.globalInvInertia
                        * glm::cross(rB, impulse);
                }
            }
        }
    }
}
